#pragma once
#include <windows.h>
#include <memory>
#include <cwchar>
#include <vector>
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

    class GameApp {
    public:
        ~GameApp ( ) {
            if ( m_comInitialized ) {
                CoUninitialize ( );
                m_comInitialized = false;
            }
        }

        void Init ( HWND hWnd )
        {
            m_hWnd = hWnd;
            m_Time.Init ( );
            m_Input.Init ( hWnd );
            InitBindings ( );

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
                // 필요 시 플레이스홀더 생성
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

            // --- 타일셋/타일맵 ---
            m_Tiles.LoadAtlas ( d3d->Device ( ) , L"assets/tiles.png" , 32 , 32 );

            // 예시 정의 (id=1: SOLID)
            engine::TileDef solid{}; solid.solid = true; solid.src = RECT{ 0,0,32,32 };
            m_Tiles.Define ( 1 , solid );

            engine::TileDef oneway{}; oneway.oneway = true; oneway.src = RECT{ 32, 0, 64, 32 };
            m_Tiles.Define ( 2 , oneway );

            // CSV 로드
            if ( m_Map.LoadCSV ( L"assets/stage01.csv" ) ) {
                m_Collision.Clear ( );
                m_Map.BuildSolidColliders ( m_Collision , m_Tiles );

                // 카메라 월드 사각형 자동 설정
                const int worldW = m_Map.W ( ) * m_Tiles.TileW ( );
                const int worldH = m_Map.H ( ) * m_Tiles.TileH ( );
                m_Cam.SetWorldRect ( 0.f , 0.f , ( float ) worldW , ( float ) worldH );
            }
        }

        // 메시지 전달 (휠/포커스 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        // 리사이즈 반영
        void OnResize ( int w , int h )
        {
            // 최소화 등으로 (0,0) 들어오는 프레임 방지
            if ( w <= 0 || h <= 0 ) return;

            // 플레이어 경계(화면 클램프 용) 갱신
            if ( m_Player ) {
                RECT rc{ 0, 0, w, h };
                m_Player->SetBounds ( rc );
            }

            // 카메라 화면 크기 갱신 (+ 픽셀 스냅 유지 시 즉시 스냅 한 번)
            m_Cam.SetScreenSize ( w , h );
            m_Cam.SnapImmediate ( );

            // 스왑체인/RTV 리사이즈
            if ( m_Renderer ) m_Renderer->Resize ( w , h );

            // 배치/디버그 드로우 뷰포트 갱신
            if ( m_Batch ) m_Batch->OnResize ( w , h );
            if ( m_Debug ) m_Debug->OnResize ( w , h );

            // DirectWrite 대상 재생성(스왑체인 백버퍼가 바뀌었으므로)
            if ( m_TextHUD ) m_TextHUD->RecreateTarget ( );

            // 충돌 콜라이더는 월드좌표라 리사이즈 시 재생성 필요 없음
            // (타일맵/월드 크기를 바꾸는 게 아니라면 그대로 유지)
        }


        // 루프 1 프레임
        bool DoOneFrame ( ) {
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

    private:
        void InitBindings ( ) {
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

        void FixedUpdate ( double fixedDt ) {
            // 타이머 감소
            m_coyoteTimer = std::max ( 0.f , m_coyoteTimer - ( float ) fixedDt );
            m_jumpBufferTimer = std::max ( 0.f , m_jumpBufferTimer - ( float ) fixedDt );

            // 점프 입력 버퍼링
            if ( m_Input.ActionPressed ( "Jump" ) || m_Input.Pressed ( VK_SPACE ) )
                m_jumpBufferTimer = 0.10f; // 100ms 버퍼

            // 착지하면 코요테 리필
            if ( m_Grounded ) m_coyoteTimer = 0.08f; // 80ms

            m_Scene.Update ( fixedDt , m_Input );

            // --- 입력/속도 ---
            const float ax = m_Input.GetAxis ( "MoveX" );
            const float ay = m_Input.GetAxis ( "MoveY" ); // ↓가 -1, ↑가 +1 (바인딩 상)
            const float moveSpeed = 180.f;
            m_Vel.x = ax * moveSpeed;

            // ↓+점프 드롭: 짧은 시간 원웨이 무시
            if ( m_Grounded && ay < -0.5f && ( m_Input.ActionPressed ( "Jump" ) || m_Input.Pressed ( VK_SPACE ) ) ) {
                m_dropThroughTimer = 0.20f;
                m_Grounded = false; // 바로 낙하 시작
            }

            // 점프 트리거 (버퍼 + 코요테)
            const bool canJump = ( m_Grounded || m_coyoteTimer > 0.f );
            if ( canJump && m_jumpBufferTimer > 0.f ) {
                m_Vel.y = -700.f;      // 점프 초기 속도
                m_Grounded = false;
                m_coyoteTimer = 0.f;
                m_jumpBufferTimer = 0.f;
            }

            // 중력
            const float g = 1200.f;
            m_Vel.y += g * ( float ) fixedDt;

            // 홀드 점프(일찍 떼면 더 빨리 낙하)
            const bool jumpHeld = m_Input.Down ( VK_SPACE ) || m_Input.ActionDown ( "Jump" );
            if ( !jumpHeld && m_Vel.y < 0.f ) {
                // 키를 뗀 상태에서 상승 중이면 추가 중력(짧은 점프)
                const float extra = g * 1.8f; // 튜닝값
                m_Vel.y += extra * ( float ) fixedDt;
            }

            // 적분
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const int prevBottom = py + ph; // ★ oneway 판정용
            float nx = ( float ) px + m_Vel.x * ( float ) fixedDt;
            float ny = ( float ) py + m_Vel.y * ( float ) fixedDt;
            RECT aabb{ ( int ) nx, ( int ) ny, ( int ) ( nx + pw ), ( int ) ( ny + ph ) };

            // 충돌 (원웨이 무시 여부 전달)
            engine::physics::CollisionReport rep{};
            const bool ignoreOneWay = ( m_dropThroughTimer > 0.f );
            m_Collision.MoveAndCollide ( aabb , m_Vel , &rep , ignoreOneWay , prevBottom );
            m_Grounded = rep.grounded;

            // 드롭 타이머 감소
            if ( m_dropThroughTimer > 0.f ) {
                m_dropThroughTimer -= static_cast< float >( fixedDt );
                if ( m_dropThroughTimer < 0.f ) m_dropThroughTimer = 0.f;
            }

            // 위치 반영
            m_Player->SetPosition ( ( float ) aabb.left , ( float ) aabb.top );

            // 애니메이터 (Idle/Walk)
            m_isMoving = ( std::fabs ( ax ) > 0.05f ) || ( std::fabs ( m_Input.GetAxis ( "MoveY" ) ) > 0.05f );
            if ( m_isMoving ) {
                if ( !m_Anim.Play ( "Walk" , false ) ) m_Anim.Play ( "Idle" , false );
            }
            else {
                m_Anim.Play ( "Idle" , false );
            }
            m_Anim.Update ( fixedDt );

            // 카메라
            if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
            m_Cam.Update ( fixedDt );
        }



        void RenderFrame ( )
        {
            m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );

            auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
            auto [ox , oy] = m_Cam.OffsetInt ( );

            // --- SpriteBatch ---
            if ( m_Batch ) {
                m_Batch->Begin ( );

                // 1) 타일맵 (가시 영역만)
                m_Map.Render ( *m_Batch , m_Tiles , ox , oy , d3d->Width ( ) , d3d->Height ( ) );

                // 2) 플레이어
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

                // 타일에서 병합된 SOLID 콜라이더
                m_Collision.DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) );

                // 널 가드 추가
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


    private:
        // --- 윈도우/코어 ---
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        // --- 렌더링 ---
        std::unique_ptr<IRenderer>               m_Renderer;       // D3D11Renderer
        std::unique_ptr<engine::D3D11SpriteBatch> m_Batch;         // 스프라이트 일괄 렌더
        std::unique_ptr<engine::D3D11DebugDraw>   m_Debug;         // 라인/박스 디버그 드로우
        std::unique_ptr<engine::DWriteTextHUD>    m_TextHUD;       // DirectWrite HUD
        Tex2D                                     m_PlayerTex{};   // 플레이어 텍스처

        // --- 월드/카메라/애니 ---
        Camera           m_Cam{};
        engine::Animator m_Anim{};
        game::Player* m_Player{ nullptr };

        // --- 타일/충돌 ---
        engine::TileSet                     m_Tiles{};
        engine::TileMap                     m_Map{};
        engine::physics::CollisionSystem    m_Collision{};

        // --- 물리 상태 ---
        engine::Vec2 m_Vel{ 0.f, 0.f };   // px/s
        bool         m_Grounded = false;

        // --- 물리/점프 보강 ---
        float m_coyoteTimer = 0.f;
        float m_jumpBufferTimer = 0.f;
        float m_dropThroughTimer = 0.f;

        // --- 기타 ---
        bool m_comInitialized = false;  // CoInitializeEx 성공 여부
        bool m_isMoving = false;
        bool m_debugDrawEnabled = true;

    };

} // namespace engine
