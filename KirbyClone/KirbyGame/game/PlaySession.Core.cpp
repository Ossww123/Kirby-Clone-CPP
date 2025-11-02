#include "game/PlaySession.h"
#include "engine/D3D11Renderer.h"
#include "engine/TextureLoader.h"
#include "engine/StringConv.h"
#include "game/AnimCSV.h"
#include "game/GameConfig.h"

namespace game {
    PlaySession::~PlaySession ( ) = default;

    void PlaySession::Initialize ( const CreateDesc& d ) {
        m_Renderer = d.renderer;
        m_Batch = d.batch;
        m_Debug = d.debug;
        m_TextHUD = d.textHUD;
        m_Scene = d.scene;

        // 플레이어/카메라 초기화 (기존 GameApp::InitPlayerAndCamera 이식)
        initPlayerAndCamera ( d.rcClient );

        // 텍스처 로드 (기존 GameApp::Init 일부 이식)
        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( d3d ) {
            if ( !m_PlayerTex.srv ) {
                engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex );
            }
            if ( !m_EnemiesTex.srv ) {
                engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/enemies.png" , &m_EnemiesTex );
            }
            if ( !m_WhiteTex.srv ) {
                engine::CreateSolidTexture1x1 ( d3d->Device ( ) , 0xFFFFFFFFu , &m_WhiteTex );
            }
        }

        if ( m_Player && m_PlayerTex.srv ) {
            m_Player->SetTexture ( m_PlayerTex );
            m_Player->SetSize ( float ( game::PLAYER_COLL_PX ) , float ( game::PLAYER_COLL_PX ) );
            m_Player->SetVisualSize ( 32.f , 32.f );
            // 애니 CSV (실패 시 스킵)
            game::LoadAnimCSV ( "assets/player_anim.csv" , m_Player->Animator ( ) , /*clear=*/true );
            if ( auto* a = m_Player->Animator ( ) ) a->Play ( "Idle" , true );
        }

        registerDefaultFactories ( );
    }

    void PlaySession::OnResize ( int sw , int sh ) {
        if ( sw <= 0 || sh <= 0 ) return;

        // 카메라 화면 크기 갱신 + 스냅 (기존 GameApp::OnResize 일부 이식)
        m_Cam.SetScreenSize ( sw , sh );
        updateCameraBoundsForWorld ( sw , sh );
        m_Cam.SnapImmediate ( );

        if ( m_Batch ) m_Batch->OnResize ( sw , sh );
        if ( m_Debug ) m_Debug->OnResize ( sw , sh );
    }

    void PlaySession::FixedUpdate ( double fixedDt , const engine::Input& input ) {
        // 1) 플레이어 FSM
        m_PlayerFSM.Step ( fixedDt , input );
        std::vector<game::PlayerEvent> evs; m_PlayerFSM.DrainEvents ( evs );
        if ( !evs.empty ( ) ) handlePlayerEvents ( evs );
        // 2) 몬스터
        updateMonsters ( fixedDt , input );
        // 3) 전투(타깃 구축 → 시스템 스텝 → 히트 적용)
        std::vector<game::ProjectileSystem::Target> projT;
        std::vector<game::HitVolumeSystem::Target> hvT;
        buildTargets ( projT , hvT );
        m_projSys.Step ( fixedDt , projT );
        m_hitSys.Step ( fixedDt , hvT );
        std::vector<game::ProjectileSystem::HitEvent> phits; m_projSys.DrainHitEvents ( phits );
        std::vector<game::HitVolumeSystem::HitEvent> hvHits; m_hitSys.DrainHitEvents ( hvHits );
        if ( !phits.empty ( ) ) applyProjectileHits ( phits );
        if ( !hvHits.empty ( ) ) applyHitVolumeHits ( hvHits );
        // 4) 애니/카메라/스폰
        if ( m_Player && m_Player->Animator ( ) ) m_Player->Animator ( )->Update ( static_cast< float >( fixedDt ) );
        if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.Update ( fixedDt );
        flushPendingSpawns ( );
        updateTransition ( fixedDt );
        // ---- Fade 진행 ----
        if ( m_fade.mode != Fade::None && m_fade.dur > 0.f ) {
            m_fade.t += float ( fixedDt );
            if ( m_fade.t >= m_fade.dur ) {
                // 끝
                m_fade.t = m_fade.dur;
                m_fade.mode = Fade::None;
            }
        }
    }

    void PlaySession::DrainPlayerEvents ( std::vector<game::PlayerEvent>& out ) {
        if ( m_pendingPlayerEvents.empty ( ) ) { out.clear ( ); return; }
        out.swap ( m_pendingPlayerEvents );
    }

    void PlaySession::StartFadeIn ( float seconds , uint32_t rgb ) { m_fade = { Fade::In,0.f,std::max ( 0.f,seconds ),rgb }; }
    void PlaySession::StartFadeOut ( float seconds , uint32_t rgb ) { m_fade = { Fade::Out,0.f,std::max ( 0.f,seconds ),rgb }; }
} // namespace game
