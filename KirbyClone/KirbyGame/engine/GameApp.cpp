#include "GameApp.h"

// === std ===
#include <cwchar>
#include <vector>
#include <algorithm>

// === engine ===
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"
#include "engine/StringConv.h"
#include "engine/Camera.h"
#include "engine/Texture.h"
#include "engine/TextureLoader.h"
#include "engine/Anim.h"
#include "game/AnimCSV.h"
#include "engine/TileSet.h"
#include "engine/TileMap.h"
#include "engine/Collision.h"
#include "engine/IRenderer.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/D3D11SpriteBatch.h"

// === game ===
#include "game/Player.h"
#include "game/Damage.h"
#include "game/Projectile.h"
#include "game/ProjectileFactory.h"
#include "game/ProjectileSystem.h"
#include "game/StageCSV.h"
#include "game/MonsterFactory.h"
#include "game/GameConfig.h"

// hit volume (spark/beam/inhale)
#include "game/HitVolume.h"
#include "game/HitVolumeFactory.h"
#include "game/HitVolumeSystem.h"
#include "game/CombatTarget.h"

namespace engine {
    GameApp::~GameApp ( )
    {
        if ( m_comInitialized ) {
            CoUninitialize ( );
            m_comInitialized = false;
        }
    }

    void GameApp::Init ( HWND hWnd )
    {
        m_hWnd = hWnd;
        m_Time.Init ( );
        m_Input.Init ( hWnd );
        InitBindings ( );

        // window size
        RECT rc; GetClientRect ( m_hWnd , &rc );
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;

        InitPlayerAndCamera ( rc );
        InitRendererUI ( hWnd , w , h );

        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        // texture / WIC load (COM initialize)
        if ( !m_comInitialized ) {
            HRESULT cohr = CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
            if ( SUCCEEDED ( cohr ) ) m_comInitialized = true;
        }

        // player texture
        if ( !LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex ) ) {
            // create placeholder if you need
            // CreateSolidTexture1x1(d3d->Device(), 0xFFFFFFFFu, &m_PlayerTex);
        }

        // monster texture
        if ( !LoadTextureWIC ( d3d->Device ( ) , L"assets/enemies.png" , &m_EnemiesTex ) ) {
            // create placeholder if you need
            // CreateSolidTexture1x1(d3d->Device(), 0xFFFFFFFFu, &m_EnemiesTex);
        }

        if ( m_PlayerTex.srv ) {
            m_Player->SetTexture ( m_PlayerTex );
            m_Player->SetSize ( float ( game::PLAYER_COLL_PX ) , float ( game::PLAYER_COLL_PX ) );
            m_Player->SetVisualSize ( 32.f , 32.f );
            if ( !game::LoadAnimCSV ( "assets/player_anim.csv" , m_Player->Animator ( ) , /*clearExisting=*/true ) ) {
                // 로드 실패 시 최소한의 폴백(원하면 로그만 남기고 스킵)
            }

            // starting clip
            m_Player->Animator ( )->Play ( "Idle" , /*restartIfSame=*/true );
        }

        RegisterDefaultFactories ( );
        InitSystems ( );
        LoadStageFromCSV ( "assets/stage01" );
    }

    LRESULT GameApp::OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
    {
        return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
    }

    void GameApp::OnResize ( int w , int h )
    {
        if ( w <= 0 || h <= 0 ) return;

        // 1) 먼저 스왑체인/RTV 리사이즈
        if ( m_Renderer ) m_Renderer->Resize ( w , h );

        // 2) 실제 백버퍼 크기 기준으로 '하나의 진실' 확보
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : w;
        const int sh = d3d ? d3d->Height ( ) : h;
        if ( sw <= 0 || sh <= 0 ) return;

        // 3) RenderSystem(프로젝션) 갱신
        m_Render.OnResize ( sw , sh );

        // 4) 카메라 화면 크기 갱신(오프셋/halfW,halfH 일치)
        m_Cam.SetScreenSize ( sw , sh );
        m_Cam.SnapImmediate ( );

        // 5) (선택) 배치/디버그가 별도라면 같은 값으로
        if ( m_Batch ) m_Batch->OnResize ( sw , sh );
        if ( m_Debug ) m_Debug->OnResize ( sw , sh );
        // if ( m_TextHUD ) m_TextHUD->RecreateTarget ( ); // (선택) 텍스트 HUD 타깃 재생성

        // 6) 플레이어의 화면 클램프 경계를 '화면 크기'로 둔다면 역시 sw,sh 사용
        if ( m_Player ) {
            RECT rc{ 0, 0, sw, sh };
            m_Player->SetBounds ( rc );
        }
    }


    bool GameApp::DoOneFrame ( )
    {
        m_Time.TickFrame ( );
        m_Input.BeginFrame ( );

        if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
            PostQuitMessage ( 0 ); return false;
        }
        if ( m_Input.ActionPressed ( "ToggleDebug" ) ) m_debugDrawEnabled = !m_debugDrawEnabled;

        if ( m_Debug ) m_Debug->BeginFrame ( );

        m_Time.CapAccumulator ( 5 );
        while ( m_Time.ShouldFixedUpdate ( ) ) {
            FixedUpdate ( m_Time.FixedDelta ( ) );
            m_Time.ConsumeFixedStep ( );
        }

        RenderFrame ( );
        return true;
    }

    bool GameApp::LoadStageFromCSV ( const char* folder )
    {
        // 0) 런타임 오브젝트 정리(핫리로드 시 기존 것 제거)
        m_Monsters.clear ( );
        m_projSys.Clear ( );

        // 1) 타일셋 + 타일맵 (엔진)
        // 스테이지 폴더 기억 (핫리로드에서 사용)
        if ( folder && *folder ) m_stageFolder = folder;
        const std::string base = m_stageFolder;

        const std::wstring tilesPng = ToWide ( base + "/tileset.png" );
        const std::wstring mapCsv = ToWide ( base + "/tilemap.csv" );

        auto * d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        m_World.LoadTileset ( d3d->Device ( ) , tilesPng , /*tileW*/game::TILE_PX , /*tileH*/game::TILE_PX );
        m_World.LoadMapCSV ( mapCsv );

        // (선택) 타일 속성 적용 → 콜라이더 재구성
        std::vector<game::TileDefCSV> tdefs;
        if ( game::LoadTileDefsCSV ( ( base + "/tiledefs.csv" ).c_str ( ) , tdefs ) && !tdefs.empty ( ) ) {
            for ( auto& r : tdefs ) {
                engine::TileDef d{}; d.solid = ( r.solid != 0 ); d.oneway = ( r.oneway != 0 );
                m_World.DefineTile ( r.id , d );
            }
            m_World.RebuildColliders ( );
        }
        else {
            // defs가 없어도 최소한 콜라이더는 갱신
            m_World.RebuildColliders ( );
        }

        // (옵션) 패럴랙스 배경: assets/<stage>/bg.png 를 찾으면 로드
        {
            std::wstring bgPng = ToWide ( base + "/bg.png" );
            Tex2D bg{};
            if ( LoadTextureWIC ( d3d->Device ( ) , bgPng.c_str ( ) , &bg ) ) {
                m_BgTex = bg;
                // 텍스처 원본 크기가 들어있다면 그대로 사용 (없다면 360x160으로 가정)
                const int srcW = ( m_BgTex.width > 0 ) ? m_BgTex.width : 360;
                const int srcH = ( m_BgTex.height > 0 ) ? m_BgTex.height : 160;
                m_bgScaledW = srcW * game::SCALE;
                m_bgScaledH = srcH * game::SCALE;
            }
            else {
                m_BgTex = {};
                m_bgScaledW = m_bgScaledH = 0;
                }
        }

        // 2) Player start
        game::PlayerStartCSV ps{};
        if ( game::LoadPlayerStartCSV ( ( base + "/player_start.csv" ).c_str ( ) , ps ) && m_Player ) {
            m_Player->SetPosition ( ps.x , ps.y );
            m_Player->Body ( ).SetVelocity ( { 0.f, 0.f } );
            m_Cam.SetWorldRect ( m_World.WorldRectPx ( ) );
            m_Cam.SetLookAt ( { ps.x, ps.y } );
            m_Cam.SnapImmediate ( );
        }

        // ProjectileSystem: Reflection new world
        m_projSys.Initialize ( m_World.WorldRectPx ( ) , &m_World.Collision ( ) );

        // Reset FSM
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) , m_Player->Animator ( ) , m_playerFsmCfg );

        // 3) 몬스터 스폰 (팩토리 경유)
        std::vector<game::MonsterCSV> mons;
        if ( game::LoadMonstersCSV ( ( base + "/monsters.csv" ).c_str ( ) , mons ) ) {
            RECT wr = m_World.WorldRectPx ( );

            for ( auto& r : mons ) {
                // 문자열 -> MonsterType 매핑
                std::string t = r.type; for ( auto& c : t ) c = ( char ) tolower ( c );
                game::MonsterType mt;
                if ( t == "waddledee" ) mt = game::MonsterType::WaddleDee;
                else if ( t == "waddledoo" ) mt = game::MonsterType::WaddleDoo;
                else if ( t == "hothead" )   mt = game::MonsterType::HotHead;
                else if ( t == "sparky" )    mt = game::MonsterType::Sparky;
                else continue;

                // CSV -> SpawnSpec
                game::SpawnSpec spec;
                spec.type = mt;
                spec.x = r.x; spec.y = r.y;
                spec.dir = ( r.dir >= 0 ) ? +1 : -1;
                spec.turnOnHitX = r.turnOnHitX;          // -1 or 0/1
                spec.turnAtEdge = r.turnAtEdge;
                spec.wakeRange = r.wakeRange;
                spec.windupMs = r.windupMs;
                spec.firePeriod = r.firePeriod;
                spec.bulletSpeed = r.bulletSpeed;
                spec.stopDuringWindup = r.stopDuringWindup;

                auto mon = game::MonsterFactory::Create ( spec.type , wr , &m_World.Collision ( ) , spec );
                if ( !mon ) continue;

                // === 임시 단일 스프라이트 적용 ===
                mon->SetSpriteSheet ( &m_EnemiesTex );
                mon->SetVisualSize ( 32.f , 32.f );
                RECT src{};
                switch ( mt ) {
                case game::MonsterType::WaddleDee: src = RECT{ 8,   8,   8 + 32,   8 + 32 }; break;
                case game::MonsterType::WaddleDoo: src = RECT{ 8,   40,  8 + 32,   40 + 32 }; break;
                case game::MonsterType::HotHead:   src = RECT{ 8,   136, 8 + 32,   136 + 32 }; break;
                case game::MonsterType::Sparky:    src = RECT{ 8,   168, 8 + 32,   168 + 32 }; break;
                default: break;
                }
                mon->SetSpriteSrc ( src );

                // 공통 콜백 부착
                mon->SetProjectileSpawner ( [ this ] ( const engine::Vec2& pos , const engine::Vec2& vel , game::ProjOwner owner ) {
                    game::ProjectileSystem::SpawnDesc sd{};
                    sd.archetype = "Star";
                    sd.owner = owner;
                    sd.pos = pos;
                    sd.dirOrVel = vel;
                    sd.treatAsDirection = false;
                    m_projSys.Spawn ( sd );
                    } );
                mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );

                m_Monsters.push_back ( std::move ( mon ) );
            }
        }
        return true;
    }

    bool GameApp::ReloadStage ( ) {
        // 디바운스: 너무 자주 호출 방지 (m_reloadCooldown은 FixedUpdate에서 감소)
        if ( m_reloadCooldown > 0.0 ) return false;
        const bool ok = LoadStageFromCSV ( m_stageFolder.c_str ( ) );
        if ( ok ) {
            m_reloadCooldown = 0.25; // 0.25초 쿨다운
            OutputDebugStringA ( "[HotReload] Stage reloaded.\n" );    
        }
         return ok;
    }

    void GameApp::InitBindings ( )
    {
        // Action
        m_Input.BindAction ( "Quit" , VK_F10 );
        m_Input.BindAction ( "Jump" , 'Z' );
        m_Input.BindAction ( "Attack" , 'X' );
        m_Input.BindAction ( "Interact" , VK_UP );
        m_Input.BindAction ( "ToggleDebug" , VK_F1 );

        // Axis
        m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
        m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
    }

    void GameApp::FixedUpdate ( double fixedDt ) {
        if ( m_reloadCooldown > 0.0 ) m_reloadCooldown -= fixedDt;
        if ( m_Input.Pressed ( VK_F5 ) ) { ReloadStage ( ); return; }
        if ( !m_Player ) return;

        StepPlayerFSM ( fixedDt );
        UpdateMonsters ( fixedDt );
        CheckContactDamage ( );

        std::vector<game::ProjectileSystem::Target> projT;
        std::vector<game::HitVolumeSystem::Target> hvT;
        BuildTargets ( projT , hvT );

        m_projSys.Step ( fixedDt , projT );
        std::vector<game::ProjectileSystem::HitEvent> phits; m_projSys.DrainHitEvents ( phits );
        ApplyProjectileHits ( phits );

        m_hitSys.Step ( fixedDt , hvT );
        std::vector<game::HitVolumeSystem::HitEvent> hvHits; m_hitSys.DrainHitEvents ( hvHits );
        ApplyHitVolumeHits ( hvHits );

        if ( m_Player && m_Player->Animator ( ) ) m_Player->Animator ( )->Update ( static_cast< float >( fixedDt ) );
        m_Cam.SetLookAt ( m_Player->Center ( ) ); m_Cam.Update ( fixedDt );
    }

    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : 0;
        const int sh = d3d ? d3d->Height ( ) : 0;
        m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );
        auto [ox , oy] = m_Cam.OffsetInt ( );

        if ( m_Batch ) {
            m_Batch->Begin ( );
            RenderParallaxBG ( ox , oy , sw , sh );   // 1) 배경
            RenderWorldBatch ( ox , oy , sw , sh );   // 2) 월드/플레이어/몬스터 (기존)
            m_Batch->End ( );
        }
        if ( m_debugDrawEnabled ) RenderDebugGridAndColliders ( ox , oy , sw , sh );
        RenderHUD ( );

        m_Renderer->EndFrame ( );
    }

    void GameApp::InitPlayerAndCamera ( const RECT& rcClient ) {
        m_Player = m_Scene.Spawn<game::Player> ( rcClient );
        m_playerFsmCfg = { .jumpSpeed = 700.f, .coyoteMs = 0.08f, .bufferMs = 0.10f, .dropMs = 0.20f };
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) , m_Player->Animator ( ) , m_playerFsmCfg );

        const int w = rcClient.right - rcClient.left;
        const int h = rcClient.bottom - rcClient.top;
        m_Cam.SetScreenSize ( w , h );
        m_Cam.SetSmoothSpeed ( 10.f );
        m_Cam.SetPixelSnap ( true );
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.SnapImmediate ( );
    }

    void GameApp::InitRendererUI ( HWND hWnd , int w , int h ) {
        m_Renderer = std::make_unique<D3D11Renderer> ( );
        if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) { PostQuitMessage ( -1 ); return; }
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        m_Batch = std::make_unique<engine::D3D11SpriteBatch> ( );
        m_Batch->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

        m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
        m_TextHUD->Initialize ( d3d->SwapChain ( ) );

        m_Debug = std::make_unique<engine::D3D11DebugDraw> ( );
        m_Debug->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );
    }

    void GameApp::RegisterDefaultFactories ( ) {
        game::MonsterFactory::RegisterDefaults ( );
        game::ProjectileFactory::RegisterDefaults ( );
        game::HitVolumeFactory::RegisterDefaults ( );

        // TODO : Transfer to CSV loader later
        // game::ProjectileFactory::LoadCSV ( "projectiles.csv" );
    }

    void GameApp::InitSystems ( ) {
        m_projSys.Initialize ( m_World.WorldRectPx ( ) , &m_World.Collision ( ) );
        m_hitSys.Initialize ( );
        m_hitSys.SetOwnerLocator ( [ this ] ( int ownerId , engine::Vec2& pos , int& fac ) { return LocateOwner ( ownerId , pos , fac ); } );
    }

    bool GameApp::LocateOwner ( int ownerId , engine::Vec2& outPos , int& outFacing ) {
        if ( m_Player && m_Player->Id ( ) == ownerId ) { outPos = m_Player->Center ( ); outFacing = m_PlayerFSM.Facing ( ); return true; }
        for ( auto& mon : m_Monsters ) if ( mon && mon->Id ( ) == ownerId ) {
            int x , y , w , h; mon->GetBounds ( x , y , w , h );
            outPos = { x + w * 0.5f, y + h * 0.5f };
            outFacing = ( mon->Velocity ( ).x >= 0.f ) ? +1 : -1;
            return true;
        }
        return false;
    }

    void GameApp::StepPlayerFSM ( double fixedDt ) {
        m_PlayerFSM.Step ( fixedDt , m_Input );
        std::vector<game::PlayerEvent> evs;
        m_PlayerFSM.DrainEvents ( evs );
        HandlePlayerEvents ( evs );
    }

    void GameApp::HandlePlayerEvents ( const std::vector<game::PlayerEvent>& evs ) {
        for ( auto& e : evs ) switch ( e.type ) {
        case game::PlayerEvent::InhaleVolume: {
            game::HitVolumeSystem::SpawnDesc sd{ "InhaleField", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::SpitStar: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "Star"; sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f, float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f }; sd.treatAsDirection = true;
            m_projSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AirPuffShot: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "AirPuff"; sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f, float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f }; sd.treatAsDirection = true;
            m_projSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AbilityFire: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const float x = ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 10.f;
            const float y = float ( py + ph * 0.5f - 4.f );
            for ( int i = 0; i < 3; ++i ) {
                const float base = 360.f , jitter = 40.f * ( i - 1 );
                game::ProjectileSystem::SpawnDesc sd{};
                sd.archetype = "FirePellet"; sd.owner = game::ProjOwner::Player;
                sd.pos = { x,y }; sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ) * ( base + jitter ), 0.f };
                sd.treatAsDirection = false; m_projSys.Spawn ( sd );
            } break;
        }
        case game::PlayerEvent::AbilitySpark: {
            game::HitVolumeSystem::SpawnDesc sd{ "SparkAura", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AbilityBeam: {
            game::HitVolumeSystem::SpawnDesc sd{ "BeamSweep", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::SwallowAbility:
        case game::PlayerEvent::AbilityGained:
        default: break;
        }
    }

    void GameApp::UpdateMonsters ( double fixedDt ) {
        for ( auto& m : m_Monsters ) if ( m ) m->Update ( fixedDt , m_Input );
    }

    void GameApp::CheckContactDamage ( ) {
        if ( !m_Player ) return;
        int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
        RECT pr{ px,py,px + pw,py + ph };
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            RECT mr{ mx,my,mx + mw,my + mh };
            if ( engine::physics::Overlap ( pr , mr ) ) {
                const float pcx = px + pw * 0.5f;
                const float mcx = mx + mw * 0.5f;
                const float dir = ( pcx < mcx ) ? -1.f : 1.f;
                game::Damage dmg; dmg.amount = 1; dmg.knockback = { dir * 260.f, -320.f };
                m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void GameApp::BuildTargets ( std::vector<game::ProjectileSystem::Target>& projT ,
                               std::vector<game::HitVolumeSystem::Target>& hvT ) {
        projT.clear ( ); hvT.clear ( );
        projT.reserve ( m_Monsters.size ( ) + 1 );
        hvT.reserve ( m_Monsters.size ( ) + 1 );

        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            projT.push_back ( { m->Id ( ), RECT{mx,my,mx + mw,my + mh}, true, /*isPlayer*/false } );

            game::HitVolumeSystem::Target t{};
            t.id = m->Id ( ); t.aabb = RECT{ mx,my,mx + mw,my + mh }; t.alive = true; t.isPlayer = false;
            t.inhalable = m->Inhalable ( );
            t.abilityGift = m->AbilityGift ( );
            hvT.push_back ( t );
        }
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            projT.push_back ( { m_Player->Id ( ), RECT{px,py,px + pw,py + ph}, true, /*isPlayer*/true } );
            game::HitVolumeSystem::Target pt{}; pt.id = m_Player->Id ( ); pt.aabb = { px,py,px + pw,py + ph };
            pt.alive = true; pt.isPlayer = true; pt.inhalable = false; pt.abilityGift = game::Ability::None;
            hvT.push_back ( pt );
        }
    }

    void GameApp::ApplyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>& phits ) {
        for ( const auto& ev : phits ) {
            game::Damage dmg{ ev.payload.damage, ev.payload.knockback };
            if ( ev.owner == game::ProjOwner::Player ) {
                for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ev.targetId ) { m->OnHit ( dmg ); break; }
            }
            else {
                if ( m_Player && m_Player->Id ( ) == ev.targetId ) m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void GameApp::ApplyHitVolumeHits ( const std::vector<game::HitVolumeSystem::HitEvent>& hvHits ) {
        for ( const auto& ev : hvHits ) {
            const bool ownerIsPlayer = ( m_Player && ev.ownerId == m_Player->Id ( ) );
            if ( ev.isCapture ) {
                if ( ownerIsPlayer ) {
                    for ( auto it = m_Monsters.begin ( ); it != m_Monsters.end ( ); ++it ) {
                        if ( *it && ( *it )->Id ( ) == ev.targetId ) {
                            const game::Ability gift = ( ev.gift != game::Ability::None ) ? ev.gift : game::Ability::None;
                            m_Monsters.erase ( it );
                            m_PlayerFSM.OnMouthCatch ( gift );
                            break;
                        }
                    }
                }
                continue;
            }
            game::Damage dmg{ ev.payload.damage, ev.payload.knockback };
            if ( ownerIsPlayer ) {
                for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ev.targetId ) { m->OnHit ( dmg ); break; }
            }
            else {
                if ( m_Player && m_Player->Id ( ) == ev.targetId ) m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void GameApp::RenderWorldBatch ( int ox , int oy , int sw , int sh )
    {
        // 1) World
        m_World.RenderVisibleScaled ( *m_Batch , ox , oy , sw , sh , game::SCALE );

        // 2) Player
        auto drawPlayer = [ & ] {
            if ( !m_Player ) return;
            const auto& tex = m_Player->Texture ( ); if ( !tex.srv ) return;
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            float vw , vh;     m_Player->GetVisualSize ( vw , vh );
            const float sx = ( ( px + pw * 0.5f ) - vw * 0.5f - ox );
            const float sy = ( ( py + ph ) - vh - oy );
            RECT src = m_Player->Animator ( )->CurrentSrc ( );
            m_Batch->Draw ( tex , sx , sy , vw * game::SCALE , vh * game::SCALE ,
                          ( src.right > src.left ) ? &src : nullptr , 0xFFFFFFFF );
            };
        drawPlayer ( );

        // 3) Monster
        auto drawMonsters = [ & ] {
            for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
                const auto* tex = m->TexturePtr ( ); if ( !tex || !tex->srv ) continue;
                int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
                float vw , vh;     m->GetVisualSize ( vw , vh );
                const float sx = ( ( mx + mw * 0.5f ) - vw * 0.5f - ox );
                const float sy = ( ( my + mh ) - vh - oy );
                RECT src = m->SpriteSrc ( );
                m_Batch->Draw ( *tex , sx , sy , vw * game::SCALE , vh * game::SCALE , &src , 0xFFFFFFFF );
            }
            };
        drawMonsters ( );
    }

    void GameApp::RenderParallaxBG ( int ox , int oy , int sw , int sh )
    {
        if ( !m_BgTex.srv ) return;

        // 스케일된 한 장의 그리기 크기 (기본 360x160 * 4 = 1440x640)
        const int BW = ( m_bgScaledW > 0 ) ? m_bgScaledW : 360 * game::SCALE;
        const int BH = ( m_bgScaledH > 0 ) ? m_bgScaledH : 160 * game::SCALE;

        // 카메라 오프셋의 일부만 반영해서 "느리게" 스크롤
        int scrollX = static_cast< int >( std::floor ( ox * m_bgParallaxX ) );
        int scrollY = static_cast< int >( std::floor ( oy * m_bgParallaxY ) );

        // 픽셀아트 지터 방지: 4배 스케일 그리드에 스냅
        if ( game::SCALE > 1 ) {
            scrollX = ( scrollX / game::SCALE ) * game::SCALE;
            scrollY = ( scrollY / game::SCALE ) * game::SCALE;
        }

        // 화면을 덮도록 좌우(필요시 상하) 타일링
        int startX = -( scrollX % BW ); if ( startX > 0 ) startX -= BW;
        int startY = -( scrollY % BH ); if ( startY > 0 ) startY -= BH;

        for ( int y = startY; y < sh; y += BH ) {
            for ( int x = startX; x < sw + BW; x += BW ) {
                m_Batch->Draw ( m_BgTex , static_cast< float >( x ) , static_cast< float >( y ) ,
                               static_cast< float >( BW ) , static_cast< float >( BH ) ,
                               /*src*/nullptr , 0xFFFFFFFF );
            }
            // 배경 한 장이 화면 높이를 이미 덮으면 한 줄만
            if ( BH >= sh ) break;
        }
    }


    void GameApp::RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh ) {
        const int GRID = game::GRID_PX;
        const int wx0 = ox , wy0 = oy , wx1 = ox + sw , wy1 = oy + sh;
        int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
        for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
        for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );
        m_World.Collision ( ).DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) , RGB ( 255 , 200 , 0 ) );
        if ( m_Player ) { int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph ); m_Debug->WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) ); }
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) m->RenderDebug ( m_Debug.get ( ) , ox , oy );
        m_projSys.DebugDraw ( *m_Debug , ox , oy );
        m_hitSys.DebugDraw ( *m_Debug , ox , oy );
        m_Debug->Flush ( );
    }

    void GameApp::RenderHUD ( )
    {
        if ( !m_TextHUD ) return;
        m_TextHUD->Begin ( );

        // FPS
        wchar_t buf[ 128 ];
        std::swprintf ( buf , _countof ( buf ) , L"FPS:%d  dt:%.3f" , m_Time.FPS ( ) , m_Time.FixedDelta ( ) );
        m_TextHUD->DrawTextLine ( buf , 8.f , 8.f );

        wchar_t st[ 64 ];
        // STATE: Movement / Action / Overlay
        std::swprintf ( st , _countof ( st ) ,
                      L"STATE  M:%S  A:%S  Z:%S" ,
                      m_PlayerFSM.MoveStateName ( ) ,
                      m_PlayerFSM.ActionStateName ( ) ,
                      m_PlayerFSM.OverlayStateName ( ) );
        m_TextHUD->DrawTextLine ( st , 8.f , 28.f );

        // FSM debug snapshot
        auto dbg = m_PlayerFSM.GetDebug ( );

        wchar_t line[ 256 ];
        std::swprintf ( line , _countof ( line ) ,
                        L"VEL: (%+07.1f, %+07.1f)  grounded(raw:%d / stable:%d)  onewayIgnore:%d" ,
                        dbg.vx , dbg.vy ,
                        dbg.groundedRaw ? 1 : 0 ,
                        dbg.groundedStable ? 1 : 0 ,
                        dbg.ignoreOneWay ? 1 : 0 );
        m_TextHUD->DrawTextLine ( line , 8.f , 48.f );

        std::swprintf ( line , _countof ( line ) ,
                      L"Timers  coyote:%.3f  buffer:%.3f  drop:%.3f  groundHold:%.3f" ,
                      dbg.coyoteT , dbg.bufferT , dbg.dropT , dbg.groundHoldT );
        m_TextHUD->DrawTextLine ( line , 8.f , 68.f );

        std::swprintf ( line , _countof ( line ) ,
                      L"AABB L:%d T:%d R:%d B:%d   prevBottom:%d" ,
                      dbg.lastAABB.left , dbg.lastAABB.top , dbg.lastAABB.right , dbg.lastAABB.bottom , dbg.prevBottom );
        m_TextHUD->DrawTextLine ( line , 8.f , 88.f );

        // tile axis under foot
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const int footX = px + pw / 2;
            const int footY = py + ph;
            const int tileSize = m_World.TileW ( );
            const int tx = footX / tileSize;
            const int ty = footY / tileSize;
            std::swprintf ( line , _countof ( line ) ,
                          L"Foot: (%d, %d)  Tile: (%d, %d)" , footX , footY , tx , ty );
            m_TextHUD->DrawTextLine ( line , 8.f , 108.f );
        }

        // camera / offset
        wchar_t cam[ 128 ]; std::swprintf ( cam , _countof ( cam ) , L"Cam LookAt: (%.1f, %.1f)" , m_Cam.GetLookAt ( ).x , m_Cam.GetLookAt ( ).y );
        m_TextHUD->DrawTextLine ( cam , 8.f , 128.f );

        wchar_t mons[ 64 ];
        std::swprintf ( mons , _countof ( mons ) , L"Monsters: %zu" , m_Monsters.size ( ) );
        m_TextHUD->DrawTextLine ( mons , 8.f , 148.f );

        wchar_t hpLine[ 64 ];
        std::swprintf ( hpLine , _countof ( hpLine ) , L"HP: %d" , m_PlayerFSM.GetDebug ( ).hp );
        m_TextHUD->DrawTextLine ( hpLine , 8.f , 168.f );

        const wchar_t* abilityName = L"None";
        switch ( dbg.ability ) {
        case game::Ability::Fire:  abilityName = L"Fire";  break;
        case game::Ability::Spark: abilityName = L"Spark"; break;
        case game::Ability::Beam:  abilityName = L"Beam";  break;
        default: break;
        }

        // Kirby flag (facing / mouthFull / ability)
        std::swprintf ( line , _countof ( line ) ,
                      L"Kirby  facing:%d  mouthFull:%d  ability:%ls" ,
                      dbg.facing , dbg.mouthFull ? 1 : 0 , abilityName );
        m_TextHUD->DrawTextLine ( line , 8.f , 188.f );

        // action timer
        std::swprintf ( line , _countof ( line ) ,
                      L"Action  inhaleT:%.2f  spitLockT:%.2f" ,
                      dbg.inhaleT , dbg.spitLockT );
        m_TextHUD->DrawTextLine ( line , 8.f , 208.f );

        m_TextHUD->End ( );
    }

} // namespace engine
