// PlaySession.Core.cpp
//
// Responsibility: Session init and fixed-step core (player FSM, monsters, combat, camera, fade).
// Non-Goals    : Renderer creation or asset policy.
// Call-Context : Called by GameApp during init and each fixed update.

#include <algorithm>
#include <cmath>

#include "game/session/PlaySession.h"
#include "engine/render/D3D11Renderer.h"
#include "engine/core/RenderSystem.h"
#include "engine/render/TextureLoader.h"
#include "engine/util/Types.h"
#include "game/data/AnimCSV.h"
#include "game/data/GameConfig.h"
#include "game/entities/monsters/WhispyWoods.h"
#include "game/entities/player/Player.h"
#include "game/data/StagePath.h" // StageJsonPathFromId
#include "protocol/SaveSchema.h" // protocol::SetCleared / ProgressT1
#include "game/render/ZOrder.h"

namespace game {

    PlaySession::~PlaySession ( ) = default;

    void PlaySession::Initialize ( const CreateDesc& d ) {
        m_Renderer = d.renderer;
        m_RenderSys = d.renderSys;
        m_TextHUD = d.textHUD;
        m_Scene = d.scene;
        m_Session = d.session;

        // Player / camera
        initPlayerAndCamera ( d.rcClient );

        // Textures (D3D11-only path stays in .cpp)
        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( d3d ) {
            if ( !m_PlayerTex.srv )    engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex );
            if ( !m_EnemiesTex.srv )   engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/enemies.png" , &m_EnemiesTex );
            if ( !m_GameOverTex.srv )  engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/ui/gameover.png" , &m_GameOverTex );
            if ( !m_HudTex.srv )       engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/ui/HUD.png" , &m_HudTex );
            if ( !m_WhiteTex.srv )     engine::CreateSolidTexture1x1 ( d3d->Device ( ) , 0xFFFFFFFFu , &m_WhiteTex );
        }

        if ( m_Player && m_PlayerTex.srv ) {
            m_Player->SetTexture ( &m_PlayerTex );
            m_Player->SetSize ( float ( game::PLAYER_COLL_PX ) , float ( game::PLAYER_COLL_PX ) );
            m_Player->SetVisualSize ( 32.f , 32.f );
            // Anim CSV (best-effort)
            game::LoadAnimCSV ( "assets/player_anim.csv" , m_Player->Animator ( ) , /*clear=*/true );
            if ( auto* a = m_Player->Animator ( ) ) a->Play ( "Idle" , true );
        }

        registerDefaultFactories ( );
    }

    void PlaySession::OnResize ( int sw , int sh ) {
        if ( sw <= 0 || sh <= 0 ) return;

        // Camera
        m_Cam.SetScreenSize ( sw , sh );
        updateCameraBoundsForWorld ( sw , sh );
        m_Cam.SnapImmediate ( );

        if ( m_RenderSys ) {
            m_RenderSys->OnResize ( sw , sh );
        }
    }

    void PlaySession::FixedUpdate ( double fixedDt , const engine::Input& input ) {
        const float fdt = static_cast< float >( fixedDt );

        // === 게임오버 화면 모드일 때 ===
        if ( m_life.gameOverScreenActive ) {
            m_life.gameOverScreenT += fdt;

            // 최소 표시 시간만큼 지나면 GameOver 완료 플래그 ON
            if ( m_life.gameOverScreenT >= m_life.gameOverScreenMin ) {
                m_life.gameOverScreenActive = false;
                m_life.gameOver = true;   // 이때부터 GameApp이 IsGameOver()로 감지
            }

            // 이 동안에는 월드/플레이어 업데이트 안 함 (화면 정지 느낌 유지)
            return;
        }

        // 0) Fade always advances, even during clear sequences
        m_fade.Update ( fixedDt );

        // 1) Clear sequence cinematic (takes over gameplay when active)
        if ( IsClearSequenceActive ( ) ) {
            updateClearFlow ( fdt );
            updateTransition ( fixedDt );
            return;
        }

        // 2) Player FSM
        m_PlayerFSM.Step ( fixedDt , input );
        std::vector<game::PlayerEvent> evs;
        m_PlayerFSM.DrainEvents ( evs );
        if ( !evs.empty ( ) ) handlePlayerEvents ( evs );

        // 3) Monsters
        updateMonsters ( fixedDt , input );

        // 4) Combat (build targets → step systems → apply)
        std::vector<game::ProjectileSystem::Target> projT;
        std::vector<game::HitVolumeSystem::Target>  hvT;
        buildTargets ( projT , hvT );

        m_projSys.Step ( fixedDt , projT );
        m_hitSys.Step ( fixedDt , hvT );

        std::vector<game::ProjectileSystem::HitEvent> phits;
        m_projSys.DrainHitEvents ( phits );
        std::vector<game::HitVolumeSystem::HitEvent>  hvHits;
        m_hitSys.DrainHitEvents ( hvHits );

        if ( !phits.empty ( ) ) applyProjectileHits ( phits );
        if ( !hvHits.empty ( ) ) applyHitVolumeHits ( hvHits );

        // HitVolume despawn
        std::vector<game::HitVolumeSystem::DespawnEvent> hvDes;
        m_hitSys.DrainDespawnEvents ( hvDes );
        if ( !hvDes.empty ( ) ) handleHitVolumeDespawns ( hvDes );

        // 4.25) Items / pickups (clear emblem etc.)
        updateItems ( fixedDt );

        // 4.5) Clear flow — before transition (e.g., emblem, autopilot, dance)
        updateClearFlow ( fdt );

        // 5) Anim / camera / spawns / transition
        if ( m_Player && m_Player->Animator ( ) )
            m_Player->Animator ( )->Update ( fdt );

        if ( m_Player )
            m_Cam.SetLookAt ( m_Player->Center ( ) );

        updateBossCameraLock ( );
        applyCamRectBlend ( fdt );
        m_Cam.Update ( fixedDt );

        for ( auto& layer : m_TileLayers ) {
            layer.Tick ( fdt );
        }

        flushPendingSpawns ( );
        updateTransition ( fixedDt );
    }

    static int iLerp ( int a , int b , float t ) { return static_cast< int >( std::lroundf ( a + ( b - a ) * t ) ); }

    engine::IntRect PlaySession::LerpRect ( const engine::IntRect& A , const engine::IntRect& B , float t ) {
        t = std::clamp ( t , 0.f , 1.f );
        engine::IntRect r;
        r.l = iLerp ( A.l , B.l , t );
        r.t = iLerp ( A.t , B.t , t );
        r.r = iLerp ( A.r , B.r , t );
        r.b = iLerp ( A.b , B.b , t );
        return r;
    }

    void PlaySession::updateItems ( double /*fixedDt*/ )
    {
        if ( !m_Player ) return;
        if ( m_Items.empty ( ) ) return;

        int px , py , pw , ph;
        m_Player->GetBounds ( px , py , pw , ph );

        const int pRight = px + pw;
        const int pBottom = py + ph;

        for ( auto& it : m_Items ) {
            if ( it.collected ) continue;

            const int iRight = it.x + it.w;
            const int iBottom = it.y + it.h;

            const bool overlap =
                ( px < iRight ) &&
                ( pRight > it.x ) &&
                ( py < iBottom ) &&
                ( pBottom > it.y );

            if ( !overlap )
                continue;

            // 현재는 ClearEmblem 하나만 처리
            if ( it.kind == ItemRuntime::Kind::ClearEmblem ) {
                it.collected = true;
                BeginClearSequence ( );
            }

            // 한 틱에 하나만 처리
            break;
        }
    }


    void PlaySession::applyCamRectBlend ( float dt ) {
        if ( !m_camBlend.active ) return;
        m_camBlend.t = std::min ( m_camBlend.t + dt , m_camBlend.dur );
        const float u = ( m_camBlend.dur > 0.f ) ? ( m_camBlend.t / m_camBlend.dur ) : 1.f;
        const float s = u * u * ( 3.f - 2.f * u ); // smoothstep
        m_Cam.SetWorldRect ( LerpRect ( m_camBlend.from , m_camBlend.to , s ) );
        if ( m_camBlend.t >= m_camBlend.dur ) m_camBlend.active = false;
    }

    // Full-world rect with small padding for nicer reveal (used as blend 'from')
    static engine::IntRect PaddedWorldRect ( const engine::IntRect& wr0 , int viewW , int viewH , int pad ) {
        engine::IntRect wr = wr0;
        const int wldW = wr.r - wr.l , wldH = wr.b - wr.t;
        if ( wldW > viewW ) { wr.l += pad; wr.r -= pad; }
        if ( wldH > viewH ) { wr.t += pad; wr.b -= pad; }
        return wr;
    }

    void PlaySession::updateBossCameraLock ( ) {
        if ( !m_hasBossArena || !m_Player ) return;

        int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
        engine::IntRect p{ px , py , px + pw , py + ph };

        const auto overl = [ ] ( const engine::IntRect& a , const engine::IntRect& b ) {
            return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
            };

        // Screen size / padding
        const int sw = m_Renderer ? m_Renderer->GetBackbufferSize ( ).w : 0;
        const int sh = m_Renderer ? m_Renderer->GetBackbufferSize ( ).h : 0;
        const int padWorld = game::TILE_PX / 2;

        if ( !m_bossCamLocked && overl ( p , m_bossArena ) ) {
            // Enter: blend Full→Arena
            m_bossCamLocked = true;
            m_camBlend.active = true; m_camBlend.t = 0.f; m_camBlend.dur = 0.6f;
            m_camBlend.from = PaddedWorldRect ( m_worldRectFull , sw , sh , padWorld );
            m_camBlend.to = m_bossArena;
        }
        if ( m_bossCamLocked && !isBossAlive ( ) ) {
            // Exit: blend Arena→Full
            m_bossCamLocked = false;
            m_camBlend.active = true; m_camBlend.t = 0.f; m_camBlend.dur = 0.6f;
            m_camBlend.from = m_bossArena;
            m_camBlend.to = PaddedWorldRect ( m_worldRectFull , sw , sh , padWorld );
        }
    }

    bool PlaySession::isBossAlive ( ) const {
        for ( const auto& up : m_Monsters ) {
            if ( !up || !up->Alive ( ) ) continue;
            if ( dynamic_cast< const WhispyWoods* >( up.get ( ) ) ) return true;
        }
        return false;
    }

    void PlaySession::DrainPlayerEvents ( std::vector<game::PlayerEvent>& out ) {
        if ( m_pendingPlayerEvents.empty ( ) ) { out.clear ( ); return; }
        out.swap ( m_pendingPlayerEvents );
    }

    void PlaySession::StartFadeIn ( float seconds , uint32_t rgb , int16_t z ) {
        game::Fade2D::Params p; p.duration = seconds; p.rgb = rgb; p.z = z;
        m_fade.StartIn ( p );
    }
    void PlaySession::StartFadeOut ( float seconds , uint32_t rgb , int16_t z ) {
        game::Fade2D::Params p; p.duration = seconds; p.rgb = rgb; p.z = z;
        m_fade.StartOut ( p );
    }

    void PlaySession::ResetLives ( int initialLives )
    {
        if ( initialLives < 0 ) initialLives = 0;

        m_life.initialLives = initialLives;
        m_life.useSharedLives = true;           // 현재는 공유 목숨만 사용
        m_life.sharedLives = initialLives;

        for ( int i = 0; i < static_cast< int >( PlayerSlot::Count ); ++i ) {
            m_life.perPlayerLives[ i ] = initialLives;
        }

        m_life.gameOver = false;
        m_life.gameOverScreenActive = false;
        m_life.gameOverScreenT = 0.f;
    }

    int PlaySession::Lives ( ) const noexcept
    {
        if ( m_life.useSharedLives ) return m_life.sharedLives;
        // per-player 모드일 때는 필요에 따라 합산/최댓값 등 선택
        int maxL = 0;
        for ( int i = 0; i < static_cast< int >( PlayerSlot::Count ); ++i )
            maxL = std::max ( maxL , m_life.perPlayerLives[ i ] );
        return maxL;
    }

    bool PlaySession::IsGameOver ( ) const noexcept
    {
        return m_life.gameOver;
    }

    void PlaySession::onPlayerDied ( PlayerSlot who )
    {
        if ( m_life.gameOver || m_life.gameOverScreenActive )
            return;

        if ( m_life.useSharedLives ) {
            if ( m_life.sharedLives > 0 ) {
                --m_life.sharedLives;

                // 부활: FSM 런타임 초기화 (앞에서 얘기한 ResetForRespawn 같은 함수)
                m_PlayerFSM.ResetForRespawn ( );
                ( void ) ReloadStage ( );
                return;
            }

            // 여기서부터는 진짜 게임오버
            m_life.gameOverScreenActive = true;
            m_life.gameOverScreenT = 0.f;

            m_PlayerFSM.BeginGameOver ( );
        }
        else {
            // === (나중용) 개별 목숨 모드 ===
            const int idx = static_cast< int >( who );
            if ( idx < 0 || idx >= static_cast< int >( PlayerSlot::Count ) ) return;

            if ( m_life.perPlayerLives[ idx ] > 0 ) {
                --m_life.perPlayerLives[ idx ];
                ( void ) ReloadStage ( );
            }
            else {
                bool allOut = true;
                for ( int i = 0; i < static_cast< int >( PlayerSlot::Count ); ++i ) {
                    if ( m_life.perPlayerLives[ i ] > 0 ) {
                        allOut = false;
                        break;
                    }
                }
                if ( allOut ) {
                    m_life.gameOver = true;
                    m_life.gameOverScreenT = 0.f;

                    m_PlayerFSM.BeginGameOver ( );
                }
            }
        }
    }

} // namespace game
