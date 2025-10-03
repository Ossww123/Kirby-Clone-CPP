#include "GameApp.h"

// 필요한 구현 헤더들
#include <cwchar>
#include <vector>
#include <algorithm>

#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"
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
            const int texW = m_PlayerTex.width , texH = m_PlayerTex.height;
            RECT full{ 0,0,texW,texH };
            engine::AnimClip idle{}; idle.frames.push_back ( { full, 0.2f } ); idle.loop = true;
            m_Anim.AddClip ( "Idle" , std::move ( idle ) );
            m_Anim.Play ( "Idle" , true );
            m_Player->SetSize ( ( float ) texW , ( float ) texH );
        }

        // 텍스트 HUD / 디버그 드로우
        m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
        m_TextHUD->Initialize ( d3d->SwapChain ( ) );
        m_Debug = std::make_unique<engine::D3D11DebugDraw> ( );
        m_Debug->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

        // --- 타일셋/타일맵 로드 ---
        m_Tiles.LoadAtlas ( d3d->Device ( ) , L"assets/tiles.png" , 32 , 32 );

        engine::TileDef solid{};  solid.solid = true;  solid.src = RECT{ 0,0,32,32 };
        engine::TileDef oneway{}; oneway.oneway = true; oneway.src = RECT{ 32,0,64,32 };
        m_Tiles.Define ( 1 , solid );
        m_Tiles.Define ( 2 , oneway );

        if ( m_Map.LoadCSV ( L"assets/stage01.csv" ) ) {
            m_Collision.Clear ( );
            m_Map.BuildSolidColliders ( m_Collision , m_Tiles );

            const int worldW = m_Map.W ( ) * m_Tiles.TileW ( );
            const int worldH = m_Map.H ( ) * m_Tiles.TileH ( );
            m_Cam.SetWorldRect ( 0.f , 0.f , ( float ) worldW , ( float ) worldH );
        }

        // 이 렌더/충돌 자원들을 RenderFrame에서 접근하기 위해 lambdas로 캡쳐하거나
        // 파일정적/싱글톤으로 간단히 유지합니다. 여기서는 파일정적 사용.
    }

    LRESULT GameApp::OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
    {
        return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
    }

    void GameApp::OnResize ( int w , int h )
    {
        if ( w <= 0 || h <= 0 ) return;

        // 플레이어 경계(화면 클램프 용) 갱신
        if ( m_Player ) {
            RECT rc{ 0,0,w,h };
            m_Player->SetBounds ( rc );
        }

        // 카메라 화면 크기 갱신
        m_Cam.SetScreenSize ( w , h );
        m_Cam.SnapImmediate ( );

        // 스왑체인/RTV 리사이즈
        if ( m_Renderer ) m_Renderer->Resize ( w , h );

        // 배치/디버그 뷰포트 갱신
        if ( m_Batch ) m_Batch->OnResize ( w , h );
        if ( m_Debug ) m_Debug->OnResize ( w , h );

        // DirectWrite 대상 재생성
        if ( m_TextHUD ) m_TextHUD->RecreateTarget ( );
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

        const float dt = static_cast< float >( fixedDt );

        // 가로 입력 → PhysicsBody에 전달
        const float axisX = m_Input.GetAxis ( "MoveX" );
        m_Player->Body ( ).SetDesiredRunAxis ( axisX );

        // 0) 타이머 감소
        m_coyoteTimer = std::max ( 0.f , m_coyoteTimer - dt );
        m_jumpBufferTimer = std::max ( 0.f , m_jumpBufferTimer - dt );
        m_dropThroughTimer = std::max ( 0.f , m_dropThroughTimer - dt );

        // 1) 입력 수집
        const float axisY = m_Input.GetAxis ( "MoveY" );

        // 점프 입력 버퍼링
        if ( m_Input.ActionPressed ( "Jump" ) || m_Input.Pressed ( VK_SPACE ) )
            m_jumpBufferTimer = m_bufferMs;

        // ↓+점프 드롭
        if ( m_Player->Body ( ).Grounded ( ) && axisY < -0.5f &&
            ( m_Input.ActionPressed ( "Jump" ) || m_Input.Pressed ( VK_SPACE ) ) )
            m_dropThroughTimer = m_dropMs;

        // 2) 가속/마찰/중력만 갱신
        m_Player->Body ( ).AdvanceKinematics ( fixedDt );

        // 코요테 리필
        if ( m_Player->Body ( ).Grounded ( ) )
            m_coyoteTimer = m_coyoteMs;

        // 점프 트리거
        if ( ( m_Player->Body ( ).Grounded ( ) || m_coyoteTimer > 0.f ) && m_jumpBufferTimer > 0.f ) {
            m_Player->Body ( ).Jump ( m_jumpSpeed );
            m_coyoteTimer = 0.f;
            m_jumpBufferTimer = 0.f;
        }

        // 3) 제안 AABB
        int prevBottom = 0;
        RECT aabb = m_Player->Body ( ).ProposeAABB ( fixedDt , &prevBottom );
        engine::Vec2 v = m_Player->Body ( ).Velocity ( );

        // 4) 충돌 해결(원웨이 포함)
        engine::physics::CollisionReport rep{};
        const bool ignoreOneWay = ( m_dropThroughTimer > 0.f );
        m_Collision.MoveAndCollide ( aabb , v , &rep , ignoreOneWay , prevBottom );

        // 5) 결과 반영
        m_Player->Body ( ).ApplyCollisionResult ( aabb , v , rep.grounded );

        // 6) 홀드-점프(저점프): 상승 중 버튼을 떼면 추가 감쇠
        const bool jumpHeld = m_Input.ActionDown ( "Jump" ) || m_Input.Down ( VK_SPACE );
        if ( !jumpHeld && m_Player->Body ( ).Velocity ( ).y < 0.f ) {
            engine::Vec2 vv = m_Player->Body ( ).Velocity ( );
            vv.y += ( m_Player->Body ( ).Params ( ).gravity * 1.8f ) * dt;
            m_Player->Body ( ).SetVelocity ( vv );
        }

        // 7) Player 렌더 캐시 싱크
        int bx , by , bw , bh; m_Player->Body ( ).GetBounds ( bx , by , bw , bh );
        m_Player->SetPosition ( ( float ) bx , ( float ) by );
        m_Player->SetSize ( ( float ) bw , ( float ) bh );

        // 8) 애니메이터/카메라
        m_isMoving = ( std::fabs ( axisX ) > 0.05f ) || ( std::fabs ( m_Input.GetAxis ( "MoveY" ) ) > 0.05f );
        if ( m_isMoving ) {
            if ( !m_Anim.Play ( "Walk" , false ) ) m_Anim.Play ( "Idle" , false );
        }
        else {
            m_Anim.Play ( "Idle" , false );
        }
        m_Anim.Update ( fixedDt );

        if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.Update ( fixedDt );
    }

    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
        m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );

        auto [ox , oy] = m_Cam.OffsetInt ( );

        // --- SpriteBatch ---
        if ( m_Batch ) {
            m_Batch->Begin ( );

            // 타일맵 (가시 영역만) — 파일 정적 map/tiles에 접근
            m_Map.Render ( *m_Batch , m_Tiles , ox , oy , d3d->Width ( ) , d3d->Height ( ) );

            // 플레이어
            if ( m_PlayerTex.srv && m_Player ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                const float x = float ( px - ox ) , y = float ( py - oy );
                const float w = float ( pw ) , h = float ( ph );
                RECT src = m_Anim.CurrentSrc ( );
                const bool hasSrc = ( src.right > src.left ) && ( src.bottom > src.top );
                m_Batch->Draw ( m_PlayerTex , x , y , w , h , hasSrc ? &src : nullptr , 0xFFFFFFFF );
            }

            m_Batch->End ( );
        }

        // --- 디버그 드로우 ---
        if ( m_debugDrawEnabled && m_Debug ) {
            const int GRID = 32;
            const int wx0 = ox , wy0 = oy , wx1 = ox + d3d->Width ( ) , wy1 = oy + d3d->Height ( );
            int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
            for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
            for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );

            // 병합된 SOLID 콜라이더
            m_Collision.DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) );

            if ( m_Player ) {
                int px , py , pw , ph;
                m_Player->GetBounds ( px , py , pw , ph );
                m_Debug->WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) );
            }

            m_Debug->Flush ( );
        }

        // --- HUD ---
        if ( m_TextHUD ) {
            m_TextHUD->Begin ( );
            wchar_t buf[ 128 ];
            std::swprintf ( buf , _countof ( buf ) , L"FPS:%d  dt:%.3f" , m_Time.FPS ( ) , m_Time.FixedDelta ( ) );
            m_TextHUD->DrawTextLine ( buf , 8.f , 8.f );
            m_TextHUD->End ( );
        }

        m_Renderer->EndFrame ( );
    }

} // namespace engine
