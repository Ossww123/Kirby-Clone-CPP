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

namespace game {

    PlaySession::~PlaySession ( ) = default;

    void PlaySession::Initialize ( const CreateDesc& d ) {
        m_Renderer = d.renderer;
        m_RenderSys = d.renderSys;
        m_TextHUD = d.textHUD;
        m_Scene = d.scene;

        // Player / camera
        initPlayerAndCamera ( d.rcClient );

        // Textures (D3D11-only path stays in .cpp)
        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( d3d ) {
            if ( !m_PlayerTex.srv )   engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex );
            if ( !m_EnemiesTex.srv )  engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/enemies.png" , &m_EnemiesTex );
            if ( !m_WhiteTex.srv )    engine::CreateSolidTexture1x1 ( d3d->Device ( ) , 0xFFFFFFFFu , &m_WhiteTex );
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
        // 1) Player FSM
        m_PlayerFSM.Step ( fixedDt , input );
        std::vector<game::PlayerEvent> evs; m_PlayerFSM.DrainEvents ( evs );
        if ( !evs.empty ( ) ) handlePlayerEvents ( evs );

        // 2) Monsters
        updateMonsters ( fixedDt , input );

        // 3) Combat (build targets → step systems → apply)
        std::vector<game::ProjectileSystem::Target> projT;
        std::vector<game::HitVolumeSystem::Target>  hvT;
        buildTargets ( projT , hvT );
        m_projSys.Step ( fixedDt , projT );
        m_hitSys.Step ( fixedDt , hvT );
        std::vector<game::ProjectileSystem::HitEvent> phits; m_projSys.DrainHitEvents ( phits );
        std::vector<game::HitVolumeSystem::HitEvent>  hvHits; m_hitSys.DrainHitEvents ( hvHits );
        if ( !phits.empty ( ) ) applyProjectileHits ( phits );
        if ( !hvHits.empty ( ) ) applyHitVolumeHits ( hvHits );

        // HitVolume despawn
        std::vector<game::HitVolumeSystem::DespawnEvent> hvDes;
        m_hitSys.DrainDespawnEvents ( hvDes );
        if ( !hvDes.empty ( ) ) handleHitVolumeDespawns ( hvDes );

        // 3.5) Clear flow — before transition
        updateClearFlow ( static_cast< float >( fixedDt ) );

        // 4) Anim / camera / spawns / transition
        if ( m_Player && m_Player->Animator ( ) ) m_Player->Animator ( )->Update ( static_cast< float >( fixedDt ) );
        if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );

        updateBossCameraLock ( );
        applyCamRectBlend ( static_cast< float >( fixedDt ) );
        m_Cam.Update ( fixedDt );

        flushPendingSpawns ( );
        updateTransition ( fixedDt );

        // 5) Fade
        if ( m_fade.mode != Fade::None && m_fade.dur > 0.f ) {
            m_fade.t += static_cast< float >( fixedDt );
            if ( m_fade.t >= m_fade.dur ) {
                m_fade.t = m_fade.dur;
                m_fade.mode = Fade::None;
            }
        }
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

    void PlaySession::StartFadeIn ( float seconds , uint32_t rgb ) { m_fade = { Fade::In , 0.f , std::max ( 0.f,seconds ) , rgb }; }
    void PlaySession::StartFadeOut ( float seconds , uint32_t rgb ) { m_fade = { Fade::Out, 0.f , std::max ( 0.f,seconds ) , rgb }; }

} // namespace game
