#include "GameApp.h"

// === std ===
#include <cwchar>
#include <vector>
#include <algorithm>
#include <cmath>

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
#include "game/StageDesc.h"
#include "game/MonsterFactory.h"
#include "game/GameConfig.h"
#include "game/StageCSV.h"

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
        LoadStage ( m_stageJsonPath.c_str ( ) );
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

        {
            RECT wr0 = m_World.WorldRectPx ( );
            if ( wr0.right > wr0.left && wr0.bottom > wr0.top ) {
                const int viewW_world = sw;
                const int viewH_world = sh;
                const int padWorld = game::TILE_PX / 2;
                RECT wr = wr0;
                const int wldW = wr.right - wr.left , wldH = wr.bottom - wr.top;
                if ( wldW > viewW_world ) { wr.left += padWorld; wr.right -= padWorld; }
                if ( wldH > viewH_world ) { wr.top += padWorld; wr.bottom -= padWorld; }
                m_Cam.SetWorldRect ( wr );
                m_Cam.SnapImmediate ( );
            }
        }

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

    bool GameApp::LoadStage ( const char* jsonPath )
    {
        // ---- 0) runtime clear ----
        m_Monsters.clear ( );
        m_projSys.Clear ( );
        m_hitSys.Clear ( );
        m_Doors.clear ( );

        // ---- 1) stage.json 읽기 ----
        game::StageDesc desc{};
        if ( !game::LoadStageDesc ( jsonPath , desc ) ) return false;

        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        if ( !d3d ) return false;

        // ---- 2) 타일셋/맵/정의 ----
        m_World.LoadTileset ( d3d->Device ( ) , ToWide ( desc.tileset ).c_str ( ) , /*cellW/H*/16 , 16 );
        m_World.SetWorldTileSize ( game::TILE_PX , game::TILE_PX ); // 월드 타일 크기(= 16*SCALE)

        int mw = 0 , mh = 0; std::vector<int> ids;
        game::LoadTileMapCSV ( desc.tilemap.c_str ( ) , mw , mh , ids );
        m_World.SetMapFromMemory ( mw , mh , ids.data ( ) );

        std::vector<game::TileDefCSV> tdefs;
        if ( game::LoadTileDefsCSV ( desc.tiledefs.c_str ( ) , tdefs ) && !tdefs.empty ( ) ) {
            const int cw = 16 , ch = 16;
            for ( auto& r : tdefs ) {
                engine::TileDef d{};
                d.solid = ( r.solid != 0 );
                d.oneway = ( r.oneway != 0 );
                if ( r.gx >= 0 && r.gy >= 0 )
                    d.src = RECT{ r.gx * cw, r.gy * ch, r.gx * cw + cw, r.gy * ch + ch };
                m_World.DefineTile ( r.id , d );
            }
        }
        m_World.RebuildColliders ( );

        // ---- 3) 배경 ----
        if ( !desc.background.empty ( ) ) {
            Tex2D bg{};
            if ( LoadTextureWIC ( d3d->Device ( ) , ToWide ( desc.background ).c_str ( ) , &bg ) ) {
                m_BgTex = bg;
            }
            else {
                m_BgTex = {};
            }
        }
        else {
            m_BgTex = {};
        }

        // ---- 4) 플레이어 시작점 & 카메라 월드 경계 ----
        game::PlayerStartCSV ps{};
        if ( game::LoadPlayerStartCSV ( desc.player_start.c_str ( ) , ps ) && m_Player ) {
            m_Player->SetPosition ( ps.x , ps.y );
            m_Player->Body ( ).SetVelocity ( { 0.f, 0.f } );

            auto* rd = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
            const int sw = rd ? rd->Width ( ) : 0;
            const int sh = rd ? rd->Height ( ) : 0;

            RECT wr0 = m_World.WorldRectPx ( );
            const int viewW_world = sw , viewH_world = sh;
            const int padWorld = game::TILE_PX / 2;

            RECT wr = wr0;
            const int ww = wr.right - wr.left , wh = wr.bottom - wr.top;
            if ( ww > viewW_world ) { wr.left += padWorld; wr.right -= padWorld; }
            if ( wh > viewH_world ) { wr.top += padWorld; wr.bottom -= padWorld; }

            m_Cam.SetWorldRect ( wr );
            m_Cam.SetLookAt ( { ps.x, ps.y } );
            m_Cam.SnapImmediate ( );
        }

        // ---- 5) 시스템 재초기화 ----
        m_projSys.Initialize ( m_World.WorldRectPx ( ) , &m_World.Collision ( ) );
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) , m_Player->Animator ( ) , m_playerFsmCfg );

        // ---- 6) 몬스터 스폰 ----
        std::vector<game::MonsterCSV> mons;
        if ( game::LoadMonstersCSV ( desc.monsters.c_str ( ) , mons ) ) {
            RECT wr = m_World.WorldRectPx ( );
            auto lower_copy = [ ] ( std::string s ) { for ( auto& c : s ) c = ( char ) std::tolower ( ( unsigned char ) c ); return s; };

            for ( auto& r : mons ) {
                std::string t = lower_copy ( r.type );
                game::MonsterType mt;
                if ( t == "waddledee" ) mt = game::MonsterType::WaddleDee;
                else if ( t == "waddledoo" ) mt = game::MonsterType::WaddleDoo;
                else if ( t == "hothead" ) mt = game::MonsterType::HotHead;
                else if ( t == "sparky" ) mt = game::MonsterType::Sparky;
                else continue;

                game::SpawnSpec spec;
                spec.type = mt; spec.x = r.x; spec.y = r.y;
                spec.dir = ( r.dir < 0 ? -1 : ( r.dir > 0 ? +1 : 0 ) );
                spec.attack = r.attack; spec.move = r.move;

                auto mon = game::MonsterFactory::Create ( spec.type , wr , &m_World.Collision ( ) , spec );
                if ( !mon ) continue;

                mon->SetSpriteSheet ( &m_EnemiesTex );
                mon->SetVisualSize ( 32.f , 32.f );
                RECT src{};
                switch ( mt ) {
                case game::MonsterType::WaddleDee: src = RECT{ 8, 8, 40, 40 }; break;
                case game::MonsterType::WaddleDoo: src = RECT{ 8, 40, 40, 72 }; break;
                case game::MonsterType::HotHead:   src = RECT{ 8, 136, 40, 168 }; break;
                case game::MonsterType::Sparky:    src = RECT{ 8, 168, 40, 200 }; break;
                default: break;
                }
                mon->SetSpriteSrc ( src );

                mon->SetProjectileSpawnerId ( [ this ] ( const std::string& arche , const engine::Vec2& pos ,
                    const engine::Vec2& vel , game::ProjOwner owner ) {
                        game::ProjectileSystem::SpawnDesc sd{};
                        sd.archetype = arche; sd.owner = owner; sd.pos = pos; sd.dirOrVel = vel; sd.treatAsDirection = false;
                        m_projSys.Spawn ( sd );
                } );
                mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );
                mon->SetHitVolumeSpawner ( [ this ] ( const std::string& arche , int ownerId , int facing , const engine::Vec2& anchor ) {
                    game::HitVolumeSystem::SpawnDesc sd{ arche, ownerId, facing, anchor };
                    m_hitSys.Spawn ( sd );
                } );

                m_Monsters.push_back ( std::move ( mon ) );
            }
        }

        // ---- 7) 도어 로드(선택) ----
        if ( !desc.doors.empty ( ) ) {
            std::vector<game::DoorCSV> doors;
            if ( game::LoadDoorsCSV ( desc.doors.c_str ( ) , doors ) ) {
                m_Doors.reserve ( doors.size ( ) );
                for ( const auto& d : doors ) {
                    Door dd;
                    dd.aabb = RECT{ d.x, d.y, d.x + d.w, d.y + d.h };
                    dd.target = d.target; // ex) "assets/stages/stage02/stage.json"
                    m_Doors.push_back ( std::move ( dd ) );
                }
            }
        }

        return true;
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
        if ( m_Input.Pressed ( VK_F5 ) && m_reloadCooldown <= 0.0 ) {
            LoadStage ( m_stageJsonPath.c_str ( ) );
            m_reloadCooldown = 0.25;
            return;
        }
        if ( m_Input.Pressed ( VK_F6 ) ) {
            m_stageJsonPath = "assets/stages/stage02/stage.json";
            LoadStage ( m_stageJsonPath.c_str ( ) );
            m_reloadCooldown = 0.25;
            return;
        }
        if ( m_Input.Pressed ( VK_F7 ) ) {
            m_stageJsonPath = "assets/stages/stage01/stage.json";
            LoadStage ( m_stageJsonPath.c_str ( ) );
            m_reloadCooldown = 0.25;
            return;
        }
        if ( !m_Player ) return;

        StepPlayerFSM ( fixedDt );
        if ( m_reloadCooldown > 0.0 ) return;
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
            RenderParallaxBG ( ox , oy , sw , sh );   // 1) Background
            RenderWorldBatch ( ox , oy , sw , sh );   // 2) World / Player / Monster
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
        case game::PlayerEvent::DoorInteract: {
            CheckDoorInteract ( );
            break; // 실제 전환되면 m_reloadCooldown이 세팅됨
        }
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
        // m_World.RenderVisibleScaled ( *m_Batch , ox , oy , sw , sh , game::SCALE );
        m_World.RenderVisible ( *m_Batch , ox , oy , sw , sh );

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

        const RECT wr = m_World.WorldRectPx ( );
        const int worldW = wr.right - wr.left;
        const int worldH = wr.bottom - wr.top;

        const int pad = game::TILE_PX / 2;

        auto calcParallaxOffset = [ & ] ( int camPos , int screenSize , int bgSize , int worldSize , int worldMin ) {
            const int camMax = std::max ( 0 , worldSize - screenSize - 2 * pad );
            const int bgMax = std::max ( 0 , bgSize - screenSize );
            const int bgPad = std::min ( pad , bgMax / 2 );
            const int bgMaxEff = std::max ( 0 , bgMax - 2 * bgPad );

            const int camEff = std::clamp ( camPos - ( worldMin + pad ) , 0 , camMax );
            const int bgOffset = ( camMax > 0 ) ? ( int ) std::lround ( ( double ) camEff * bgMaxEff / camMax ) : 0;

            return ( bgOffset + bgPad ) / game::SCALE;
            };

        const int srcLeft = calcParallaxOffset ( ox , sw , m_BgTex.width * game::SCALE , worldW , wr.left );
        const int srcTop = calcParallaxOffset ( oy , sh , m_BgTex.height * game::SCALE , worldH , wr.top );

        const int viewW_tex = sw / game::SCALE;
        const int viewH_tex = sh / game::SCALE;

        RECT src{
            std::clamp ( srcLeft, 0, std::max ( 0, m_BgTex.width - viewW_tex ) ),
            std::clamp ( srcTop, 0, std::max ( 0, m_BgTex.height - viewH_tex ) ),
            0, 0
        };
        src.right = src.left + viewW_tex;
        src.bottom = src.top + viewH_tex;

        m_Batch->Draw ( m_BgTex , 0.f , 0.f , ( float ) sw , ( float ) sh , &src , 0xFFFFFFFF );
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
        // Doors
        for ( const auto& d : m_Doors ) {
            const int x = d.aabb.left , y = d.aabb.top;
            const int w = d.aabb.right - d.aabb.left;
            const int h = d.aabb.bottom - d.aabb.top;
            m_Debug->WorldRect ( x , y , w , h , ox , oy , RGB ( 80 , 160 , 255 ) );
        }
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

    void GameApp::CheckDoorInteract ( )
    {
        if ( !m_Player || m_Doors.empty ( ) ) return;

        int px , py , pw , ph;
        m_Player->GetBounds ( px , py , pw , ph );
        RECT pr{ px, py, px + pw, py + ph };

        for ( const auto& d : m_Doors ) {
            if ( engine::physics::Overlap ( pr , d.aabb ) ) {
                // stage 전환
                if ( !d.target.empty ( ) ) {
                    m_stageJsonPath = d.target;
                    if ( !LoadStage ( m_stageJsonPath.c_str ( ) ) ) {
                        OutputDebugStringA ( "[Door] Stage load FAILED: check target path.\n" );
                    }
                    m_reloadCooldown = 0.25;
                }
                return;
            }
        }
    }


} // namespace engine
