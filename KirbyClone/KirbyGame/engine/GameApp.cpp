#include "GameApp.h"

// 필요한 구현 헤더들
#include <cwchar>
#include <vector>
#include <algorithm>

#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"
#include "engine/StringConv.h"
#include "engine/Camera.h"
#include "engine/Texture.h"
#include "engine/TextureLoader.h"
#include "engine/Anim.h"
#include "engine/TileSet.h"
#include "engine/TileMap.h"
#include "engine/Collision.h"
#include "engine/IRenderer.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/D3D11SpriteBatch.h"
#include "game/Player.h"
#include "game/Damage.h"
#include "game/Projectile.h"
#include "game/StageCSV.h"
#include "game/MonsterFactory.h"
#include "game/WaddleDee.h"
#include "game/WaddleDoo.h"


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
        InitBindings ( );           // 키 매핑

        // 창 크기
        RECT rc; GetClientRect ( m_hWnd , &rc );
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;

        // 씬/플레이어
        m_Player = m_Scene.Spawn<game::Player> ( rc );
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) ,
                         /*anim*/ m_Player->Animator ( ) ,     // FSM이 Player의 Animator를 제어
                         { .jumpSpeed = 700.f, .coyoteMs = 0.08f, .bufferMs = 0.10f, .dropMs = 0.20f } );

        // 카메라
        m_Cam.SetScreenSize ( w , h );
        m_Cam.SetSmoothSpeed ( 10.f );
        m_Cam.SetPixelSnap ( true );
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.SnapImmediate ( );

        // D3D11 렌더러
        m_Renderer = std::make_unique<D3D11Renderer> ( );
        if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) {
            PostQuitMessage ( -1 );
            return;
        }
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        // SpriteBatch
        m_Batch = std::make_unique<engine::D3D11SpriteBatch> ( );
        m_Batch->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

        // 텍스처/WIC 로드 (COM 초기화)
        HRESULT cohr = CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
        if ( SUCCEEDED ( cohr ) ) m_comInitialized = true;

        // 플레이어 텍스처
        if ( !LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex ) ) {
            // 필요 시 플레이스홀더 생성 가능
            // CreateSolidTexture1x1(d3d->Device(), 0xFFFFFFFFu, &m_PlayerTex);
        }

        if ( m_PlayerTex.srv ) {
            // 텍스처 → Player
            m_Player->SetTexture ( m_PlayerTex );
            // 스프라이트 실제 크기
            m_Player->SetSize ( 32.f , 32.f );
            
            // 좌상단 (sx,sy)에서 가로로 count개를 자르는 스트립 생성 헬퍼
            auto makeStrip = [ & ] ( int sx , int sy , int fw , int fh , int count , float dur , bool loop )->engine::AnimClip {
                engine::AnimClip c; c.loop = loop;
                for ( int i = 0; i < count; ++i ) {
                    c.frames.push_back ( { RECT{ sx + i * fw, sy, sx + ( i + 1 ) * fw, sy + fh }, dur } );
                }
                return c;
            };
            
            // ===== 시트 레이아웃 (픽셀) : 셀 32x32 =====
            // IDLE: (8, 8)부터 2개
            m_Player->Animator ( )->AddClip ( "Idle" , makeStrip ( 8 , 8 , 32 , 32 , 2 , 0.20f , /*loop=*/true ) );
            // WALK: (8, 72)부터 6개
            m_Player->Animator ( )->AddClip ( "Walk" , makeStrip ( 8 , 72 , 32 , 32 , 6 , 0.10f , /*loop=*/true ) );
            // JUMP: (8, 136)부터 1개
            m_Player->Animator ( )->AddClip ( "Jump" , makeStrip ( 8 , 136 , 32 , 32 , 1 , 0.12f , /*loop=*/false ) );
            // FALL: (40, 136)부터 6개  (40→72→104→…)
            m_Player->Animator ( )->AddClip ( "Fall" , makeStrip ( 40 , 136 , 32 , 32 , 6 , 0.12f , /*loop=*/true ) );
            
            // 시작 클립
            m_Player->Animator ( )->Play ( "Idle" , /*restartIfSame=*/true );
        }
            
        // 텍스트 HUD / 디버그 드로우
        m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
        m_TextHUD->Initialize ( d3d->SwapChain ( ) );
        m_Debug = std::make_unique<engine::D3D11DebugDraw> ( );
        m_Debug->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

        // 타일셋 로드 + 타일 정의 + 맵 로드
        m_World.LoadTileset ( d3d->Device ( ) , L"assets/tiles.png" , 32 , 32 );

        // 예시 타일 정의
        engine::TileDef solid{};  solid.solid   = true; solid.src  = RECT{ 0, 0, 32, 32 };
        engine::TileDef oneway{}; oneway.oneway = true; oneway.src = RECT{ 32, 0, 64, 32 };
        m_World.DefineTile ( 1 , solid );
        m_World.DefineTile ( 2 , oneway );

        // 맵 로드 + 콜라이더
        if ( m_World.LoadMapCSV ( L"assets/stage01/tilemap.csv" ) ) {
            m_World.RebuildColliders ( );

            // 카메라 월드 사각형 자동 설정
            RECT wr = m_World.WorldRectPx ( );
            m_Cam.SetWorldRect ( ( float ) wr.left , ( float ) wr.top , ( float ) wr.right , ( float ) wr.bottom );

            // === 몬스터 팩토리 등록 + 스폰 ===
            game::MonsterFactory::RegisterDefaults ( ); // 한 번만

            LoadStageFromCSV ( "assets/stage01" );
        }
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
            PostQuitMessage ( 0 );
            return false;
        }

        if ( m_Input.ActionPressed ( "ToggleDebug" ) )
            m_debugDrawEnabled = !m_debugDrawEnabled;
        if ( m_Debug ) m_Debug->BeginFrame ( );

        // 고정 업데이트 (스파이럴 방지)
        int steps = 0;
        constexpr int MAX_STEPS = 5;
        while ( m_Time.ShouldFixedUpdate ( ) && steps < MAX_STEPS ) {
            FixedUpdate ( m_Time.FixedDelta ( ) );
            m_Time.ConsumeFixedStep ( );
            ++steps;
        }

        RenderFrame ( );
        return true;
    }

    bool GameApp::LoadStageFromCSV ( const char* folder )
    {
        // 1) 타일셋 + 타일맵 (엔진)
        const std::string base ( folder );
        const std::wstring tilesPng = ToWide ( base + "/tileset.png" );
        const std::wstring mapCsv = ToWide ( base + "/tilemap.csv" );

        auto * d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        m_World.LoadTileset ( d3d->Device ( ) , tilesPng , /*tileW*/32 , /*tileH*/32 );
        m_World.LoadMapCSV ( mapCsv );

        // (선택) 타일 속성 적용 → 콜라이더 재구성
        std::vector<game::TileDefCSV> tdefs;
        if ( game::LoadTileDefsCSV ( ( std::string ( folder ) + "/tiledefs.csv" ).c_str ( ) , tdefs ) && !tdefs.empty ( ) ) {
            for ( auto& r : tdefs ) {
                engine::TileDef d{}; d.solid = ( r.solid != 0 ); d.oneway = ( r.oneway != 0 );
                m_World.DefineTile ( r.id , d );
            }
            m_World.RebuildColliders ( );
        }

        // 2) 플레이어 시작
        game::PlayerStartCSV ps{};
        if ( game::LoadPlayerStartCSV ( ( std::string ( folder ) + "/player_start.csv" ).c_str ( ) , ps ) && m_Player ) {
            m_Player->SetPosition ( ps.x , ps.y );
            m_Cam.SetWorldRect ( m_World.WorldRectPx ( ) );
            m_Cam.SetLookAt ( { ps.x, ps.y } );
            m_Cam.SnapImmediate ( );
        }

        // 3) 몬스터 스폰
        std::vector<game::MonsterCSV> mons;
        if ( game::LoadMonstersCSV ( ( std::string ( folder ) + "/monsters.csv" ).c_str ( ) , mons ) ) {
            RECT wr = m_World.WorldRectPx ( );
            for ( auto& r : mons ) SpawnMonsterFromRow ( wr , r );
        }
        return true;
    }

    void GameApp::SpawnMonsterFromRow ( const RECT& wr , const game::MonsterCSV& r )
    {
        std::unique_ptr<game::Monster> mon;
        std::string t = r.type; for ( auto& c : t ) c = ( char ) tolower ( c );

        if ( t == "waddledee" ) {
            game::WaddleDee::Config cfg;
            cfg.dir = r.dir;
            if ( r.turnOnHitX >= 0 ) cfg.turnOnHitX = ( r.turnOnHitX != 0 );
            if ( r.turnAtEdge >= 0 ) cfg.turnAtEdge = ( r.turnAtEdge != 0 );
            mon = std::make_unique<game::WaddleDee> ( wr , &m_World.Collision ( ) , cfg );
        }
        else if ( t == "waddledoo" ) {
            game::WaddleDoo::Config cfg;
            cfg.dir = r.dir;
            if ( r.turnOnHitX >= 0 )       cfg.turnOnHitX = ( r.turnOnHitX != 0 );
            if ( r.turnAtEdge >= 0 )       cfg.turnAtEdge = ( r.turnAtEdge != 0 );
            if ( r.wakeRange >= 0 )        cfg.wakeRange = r.wakeRange;
            if ( r.windupMs >= 0 )         cfg.windupMs = r.windupMs;
            if ( r.firePeriod >= 0 )       cfg.firePeriod = r.firePeriod;
            if ( r.bulletSpeed >= 0 )      cfg.bulletSpeed = r.bulletSpeed;
            if ( r.stopDuringWindup >= 0 ) cfg.stopDuringWindup = ( r.stopDuringWindup != 0 );
            mon = std::make_unique<game::WaddleDoo> ( wr , &m_World.Collision ( ) , cfg );
        }
        else return; // 알 수 없는 타입

        // 위치
        mon->SetPosition ( r.x , r.y );

        // 콜백(공통)
        mon->SetProjectileSpawner ( [ this ] ( const engine::Vec2& pos , const engine::Vec2& vel , game::ProjOwner owner ) {
            game::Projectile::Cfg pcfg; pcfg.width = 8; pcfg.height = 8;
            auto p = std::make_unique<game::Projectile> ( m_World.WorldRectPx ( ) , &m_World.Collision ( ) , owner , pcfg );
            p->Fire ( pos , vel );
            m_Projectiles.push_back ( std::move ( p ) );
        } );
        mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );

        m_Monsters.push_back ( std::move ( mon ) );
    }


    void GameApp::InitBindings ( )
    {
        // 액션
        m_Input.BindAction ( "Quit" , VK_F10 );
        m_Input.BindAction ( "Jump" , VK_SPACE );
        m_Input.BindAction ( "Attack" , 'J' );
        m_Input.BindAction ( "Dash" , 'K' );
        m_Input.BindAction ( "ToggleDebug" , VK_F1 );

        // 축
        m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
        m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
    }

    void GameApp::FixedUpdate ( double fixedDt )
    {
        if ( !m_Player ) return;

        m_PlayerFSM.Step ( fixedDt , m_Input );

        // 바라보는 방향 추적: 속도/입력 기준
        if ( std::fabs ( m_Input.GetAxis ( "MoveX" ) ) > 0.1f )
            m_facing = ( m_Input.GetAxis ( "MoveX" ) >= 0.f ) ? +1 : -1;
        else {
            // 입력이 없다면, 실제 속도로 추정
            const auto dbg = m_PlayerFSM.GetDebug ( );
            if ( std::fabs ( dbg.vx ) > 1.f ) m_facing = ( dbg.vx >= 0.f ) ? +1 : -1;
        }

        // ── 발사 입력(J) ──
        if ( m_Input.ActionPressed ( "Attack" ) ) {
            // 플레이어 중앙에서 발사
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            engine::Vec2 start{ ( float ) ( px + pw / 2 ), ( float ) ( py + ph / 2 ) };

            // 방향에 따라 속도
            game::Projectile::Cfg pcfg;
            pcfg.width = 8; pcfg.height = 8; pcfg.speed = 520.f;

            // 월드 경계
            RECT wr = m_World.WorldRectPx ( );

            auto proj = std::make_unique<game::Projectile> ( wr , &m_World.Collision ( ) , game::ProjOwner::Player , pcfg );
            engine::Vec2 vel{ ( float ) m_facing * pcfg.speed, 0.f };
            proj->Fire ( start , vel );
            m_Projectiles.push_back ( std::move ( proj ) );
        }

        // 몬스터 업데이트
        for ( auto& m : m_Monsters ) if ( m ) m->Update ( fixedDt , m_Input );

        // ── 투사체 업데이트 ──
        for ( auto& p : m_Projectiles ) if ( p ) p->Update ( fixedDt , m_Input );

        // 접촉 데미지 체크
        int px , py , pw , ph;
        m_Player->GetBounds ( px , py , pw , ph );
        RECT pr{ px, py, px + pw, py + ph };

        for ( auto& m : m_Monsters ) if ( m ) {
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            RECT mr{ mx, my, mx + mw, my + mh };
            if ( engine::physics::Overlap ( pr , mr ) ) {
                // 몬스터 기준으로 넉백 방향 계산
                const float pcx = px + pw * 0.5f;
                const float mcx = mx + mw * 0.5f;
                const float dir = ( pcx < mcx ) ? -1.f : 1.f; // 플레이어가 왼쪽이면 왼쪽으로 튕김
                game::Damage dmg;
                dmg.amount = 1;
                dmg.knockback = engine::Vec2{ dir * 260.f, -320.f }; // 튜닝 가능
                m_PlayerFSM.ApplyDamage ( dmg );
            }
        }

        // ── 몬스터 피격 판정 ──
        for ( auto& p : m_Projectiles ) {
            if ( p && p->Alive ( ) && p->Owner ( ) == game::ProjOwner::Player )
            {
                int px , py , pw , ph; p->GetBounds ( px , py , pw , ph );
                RECT pr{ px, py, px + pw, py + ph };

                for ( auto& m : m_Monsters ) {
                    if ( m && m->Alive ( ) )
                    {
                        int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
                        RECT mr{ mx, my, mx + mw, my + mh };
                        if ( engine::physics::Overlap ( pr , mr ) ) {
                            // 명중!
                            const float dir = ( px < mx ) ? -1.f : +1.f;
                            game::Damage dmg;
                            dmg.amount = 1;
                            dmg.knockback = engine::Vec2{ dir * 300.f, -200.f };
                            m->OnHit ( dmg );
                            p->Kill ( ); // 투사체 소멸
                            break;
                        }
                    }
                }
            }
        }

        // === Enemy Projectile vs Player ===
        for ( auto& p : m_Projectiles )
            if ( p && p->Alive ( ) && p->Owner ( ) == game::ProjOwner::Enemy )
            {
                int bx , by , bw , bh; p->GetBounds ( bx , by , bw , bh );
                RECT br{ bx,by,bx + bw,by + bh };

                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                RECT pr{ px,py,px + pw,py + ph };

                if ( engine::physics::Overlap ( br , pr ) ) {
                    const float dir = ( ( px + pw * 0.5f ) < ( bx + bw * 0.5f ) ) ? -1.f : +1.f;
                    game::Damage dmg;
                    dmg.amount = 1;
                    dmg.knockback = engine::Vec2{ dir * 260.f, -320.f };
                    m_PlayerFSM.ApplyDamage ( dmg );
                    p->Kill ( );
                }
            }


        // ── 죽은 투사체 정리 ──
        m_Projectiles.erase (
            std::remove_if ( m_Projectiles.begin ( ) , m_Projectiles.end ( ) ,
                [ ] ( const std::unique_ptr<game::Projectile>& p ) { return !p || !p->Alive ( ); } ) ,
            m_Projectiles.end ( )
        );

        // 애니메이터 (플레이어)
        if ( m_Player && m_Player->Animator ( ) )
            m_Player->Animator ( )->Update ( static_cast< float >( fixedDt ) );

        // 카메라
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.Update ( fixedDt );
    }


    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : 0;
        const int sh = d3d ? d3d->Height ( ) : 0;

        // BeginFrame은 Color 타입을 받도록 수정
        Color clear{ 0.09f, 0.11f, 0.125f, 1.0f };
        m_Renderer->BeginFrame ( clear );

        auto [ox , oy] = m_Cam.OffsetInt ( );

        // --- SpriteBatch ---
        if ( m_Batch ) {
            m_Batch->Begin ( );

            // 1) 타일맵 (가시 영역만)
            m_World.RenderVisible ( *m_Batch , ox , oy , sw , sh );

            // 2) 플레이어
            if ( m_Player ) {
                const auto& tex = m_Player->Texture ( );
                if ( tex.srv ) {
                    int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                    const float x = float ( px - ox ) , y = float ( py - oy );
                    const float w = float ( pw ) , h = float ( ph );

                    RECT src = m_Player->Animator ( )->CurrentSrc ( );
                    const bool hasSrc = ( src.right > src.left ) && ( src.bottom > src.top );
                    m_Batch->Draw ( tex , x , y , w , h , hasSrc ? &src : nullptr , 0xFFFFFFFF );
                }
            }

            m_Batch->End ( );
        }

        // --- 디버그 드로우 ---
        if ( m_debugDrawEnabled && m_Debug ) {
            const int GRID = 32;
            const int wx0 = ox , wy0 = oy , wx1 = ox + sw , wy1 = oy + sh;
            int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
            for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
            for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );

            // 병합된 SOLID 콜라이더
            m_World.Collision ( ).DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) );

            if ( m_Player ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                m_Debug->WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) );
            }

            // 몬스터
            for ( auto& m : m_Monsters )
                if ( m && m->Alive ( ) )
                    m->RenderDebug ( m_Debug.get ( ) , ox , oy );

            // 투사체
            for ( auto& p : m_Projectiles )
                if ( p && p->Alive ( ) ) {
                    int x , y , w , h; p->GetBounds ( x , y , w , h );
                    const auto color = ( p->Owner ( ) == game::ProjOwner::Player ) ? RGB ( 255 , 230 , 90 ) : RGB ( 120 , 200 , 255 );
                    m_Debug->WorldRect ( x , y , w , h , ox , oy , color );
                }

            m_Debug->Flush ( );
        }

        // --- HUD ---
        if ( m_TextHUD ) {
            m_TextHUD->Begin ( );

            // 기존: FPS/STATE
            wchar_t buf[ 128 ];
            std::swprintf ( buf , _countof ( buf ) , L"FPS:%d  dt:%.3f" , m_Time.FPS ( ) , m_Time.FixedDelta ( ) );
            m_TextHUD->DrawTextLine ( buf , 8.f , 8.f );

            wchar_t st[ 64 ];
            std::swprintf ( st , _countof ( st ) , L"STATE: %S" , m_PlayerFSM.StateName ( ) );
            m_TextHUD->DrawTextLine ( st , 8.f , 28.f );

            // 추가: FSM 디버그 스냅샷
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

            // 발밑 타일 좌표(타일 32px 가정)
            if ( m_Player ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                const int footX = px + pw / 2;
                const int footY = py + ph;         // 발바닥 y
                const int tileSize = 32;
                const int tx = footX / tileSize;
                const int ty = footY / tileSize;
                std::swprintf ( line , _countof ( line ) ,
                              L"Foot: (%d, %d)  Tile: (%d, %d)" , footX , footY , tx , ty );
                m_TextHUD->DrawTextLine ( line , 8.f , 108.f );
            }

            // 카메라/오프셋 확인
            wchar_t cam[128]; std::swprintf(cam, _countof(cam), L"Cam LookAt: (%.1f, %.1f)", m_Cam.GetLookAt().x, m_Cam.GetLookAt().y);
            m_TextHUD->DrawTextLine(cam, 8.f, 128.f);

            wchar_t mons[ 64 ];
            std::swprintf ( mons , _countof ( mons ) , L"Monsters: %zu" , m_Monsters.size ( ) );
            m_TextHUD->DrawTextLine ( mons , 8.f , 148.f );

            wchar_t hpLine[ 64 ];
            std::swprintf ( hpLine , _countof ( hpLine ) , L"HP: %d" , m_PlayerFSM.GetDebug ( ).hp );
            m_TextHUD->DrawTextLine ( hpLine , 8.f , 168.f );

            m_TextHUD->End ( );
        }

        m_Renderer->EndFrame ( );
    }


} // namespace engine
