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

        void Init ( HWND hWnd ) {
            m_hWnd = hWnd;
            m_Time.Init ( );
            m_Input.Init ( hWnd );
            InitBindings ( );

            // 클라이언트 크기
            RECT rc; GetClientRect ( m_hWnd , &rc );
            const int w = rc.right - rc.left;
            const int h = rc.bottom - rc.top;

            // 씬/플레이어
            m_Player = m_Scene.Spawn<game::Player> ( rc );

            // 카메라 설정
            m_Cam.SetScreenSize ( w , h );
            m_Cam.SetWorldRect ( 0.f , 0.f , 3000.f , 1600.f ); // 데모용 월드 크기
            m_Cam.SetSmoothSpeed ( 10.f );
            m_Cam.SetPixelSnap ( true );
            m_Cam.SetLookAt ( m_Player->Center ( ) );
            m_Cam.SnapImmediate ( );

            // 간단한 바닥/벽 배치
            m_StaticSolids.push_back ( RECT{ -2000, 500,  4000, 560 } ); // 바닥(두꺼운 플랫폼)
            m_StaticSolids.push_back ( RECT{ 300,  360,   600, 380 } ); // 발판
            m_StaticSolids.push_back ( RECT{ 800,  440,  1200, 460 } ); // 발판
            m_StaticSolids.push_back ( RECT{ -100,  300,  -80,  520 } );  // 왼쪽 기둥(벽)

            // D3D11 렌더러 생성
            m_Renderer = std::make_unique<D3D11Renderer> ( );
            if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) {
                PostQuitMessage ( -1 );
                return;
            }

            // 스프라이트 렌더러 초기화
            auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
            m_Batch = std::make_unique<engine::D3D11SpriteBatch> ( );
            m_Batch->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

            // WIC 초기화 + 텍스처 로드
            HRESULT cohr = CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
            if ( SUCCEEDED ( cohr ) ) m_comInitialized = true; // 이미 초기화되어 있으면 S_FALSE
            // 실행 디렉터리 기준 경로 (VS의 작업 디렉터리를 프로젝트 루트로 맞추는 것을 권장)
            if ( !LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex ) ) {
                // TODO: 플레이스홀더 생성이나 오류 로그를 원하면 여기에 처리
            }

            // 텍스처 로드 후 (성공 시)
            if ( m_PlayerTex.srv ) {
                const int texW = m_PlayerTex.width;
                const int texH = m_PlayerTex.height;

                // (A) 단일 이미지일 때: Idle 한 프레임
                RECT full{ 0, 0, texW, texH };
                engine::AnimClip idle{};
                idle.frames.push_back ( { full, 0.2f } );
                idle.loop = true;
                m_Anim.AddClip ( "Idle" , std::move ( idle ) );

                // (B) 시트가 있을 때
                // 예: 가로 6프레임, 한 칸 32x32, 12fps
                // const int cellW = 32, cellH = 32, count = 6; const float fps = 12.f;
                // engine::AnimClip walk = engine::Animator::MakeRowClip(0, 0, cellW, cellH, count, fps, true);
                // m_Anim.AddClip("Walk", std::move(walk));
                // m_Player->SetSize((float)cellW, (float)cellH); // 비율 유지하려면 프레임 크기로 맞추기

                // 시트가 아직 없으면 원본 크기로
                m_Player->SetSize ( ( float ) texW , ( float ) texH );

                m_Anim.Play ( "Idle" , true );
            }

            m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
            if ( !m_TextHUD->Initialize ( d3d->SwapChain ( ) ) ) {
                // 실패해도 치명적이진 않지만, 로그 남기고 넘어가도 OK
            }

            m_Debug = std::make_unique<engine::D3D11DebugDraw> ( );
            m_Debug->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );
        }

        // 메시지 전달 (휠/포커스 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        // 리사이즈 반영
        void OnResize ( int w , int h ) {
            if ( m_Player ) {
                RECT rc{ 0,0,w,h };
                m_Player->SetBounds ( rc );
            }
            m_Cam.SetScreenSize ( w , h );

            if ( m_Renderer ) m_Renderer->Resize ( w , h );
            if ( m_Debug ) m_Debug->OnResize ( w , h );
            if ( m_TextHUD ) m_TextHUD->RecreateTarget ( );
            if ( m_Batch ) m_Batch->OnResize ( w , h );
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
            m_Scene.Update ( fixedDt , m_Input );

            // 입력 → 속도
            const float ax = m_Input.GetAxis ( "MoveX" );
            const float moveSpeed = 180.f;
            m_Vel.x = ax * moveSpeed;

            if ( m_Grounded && ( m_Input.ActionPressed ( "Jump" ) || m_Input.Pressed ( VK_SPACE ) ) ) {
                m_Vel.y = -700.f;
                m_Grounded = false;
            }

            // 중력
            const float g = 1200.f;
            m_Vel.y += g * static_cast< float >( fixedDt );

            // 적분
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            float nx = static_cast< float >( px ) + m_Vel.x * static_cast< float >( fixedDt );
            float ny = static_cast< float >( py ) + m_Vel.y * static_cast< float >( fixedDt );
            RECT aabb{ ( int ) nx, ( int ) ny, ( int ) ( nx + pw ), ( int ) ( ny + ph ) };

            // 충돌 해결(부호 수정 포함)
            m_Grounded = false;
            for ( const RECT& s : m_StaticSolids ) {
                if ( !engine::coll::Overlap ( aabb , s ) ) continue;
                POINT mtv = engine::coll::ResolveMTV ( aabb , s );
                aabb.left += mtv.x; aabb.right += mtv.x;
                aabb.top += mtv.y; aabb.bottom += mtv.y;

                if ( mtv.y < 0 ) { m_Grounded = true; m_Vel.y = 0.f; }
                else if ( mtv.y > 0 ) { m_Vel.y = 0.f; }
                if ( mtv.x != 0 ) m_Vel.x = 0.f;
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



        void RenderFrame ( ) {
            m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );

            auto [ox , oy] = m_Cam.OffsetInt ( );

            if ( m_Batch && m_PlayerTex.srv && m_Player ) {
                m_Batch->Begin ( );

                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                const float x = float ( px - ox ) , y = float ( py - oy );
                const float w = float ( pw ) , h = float ( ph );

                RECT src = m_Anim.CurrentSrc ( );
                const bool hasSrc = ( src.right > src.left ) && ( src.bottom > src.top );
                m_Batch->Draw ( m_PlayerTex , x , y , w , h , hasSrc ? &src : nullptr , 0xFFFFFFFF );

                m_Batch->End ( );
            }

            // --- D3D DebugDraw (라인/박스) ---
            if ( m_debugDrawEnabled && m_Debug ) {
                auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
                const int GRID = 32;
                const int wx0 = ox , wy0 = oy , wx1 = ox + d3d->Width ( ) , wy1 = oy + d3d->Height ( );
                int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;

                for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
                for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );

                for ( const RECT& s : m_StaticSolids ) {
                    m_Debug->WorldRect ( s.left , s.top ,
                                       s.right - s.left , s.bottom - s.top ,
                                       ox , oy , RGB ( 255 , 60 , 60 ) );
                }

                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                m_Debug->WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) );

                m_Debug->Flush ( );
            }


            // --- DirectWrite HUD ---
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
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        std::unique_ptr<IRenderer>            m_Renderer;
        std::unique_ptr<engine::D3D11DebugDraw> m_Debug;
        std::unique_ptr<engine::DWriteTextHUD> m_TextHUD;
        std::unique_ptr<engine::D3D11SpriteBatch> m_Batch;
        Tex2D                                  m_PlayerTex{};

        Camera          m_Cam{};
        game::Player* m_Player{};
        engine::Animator m_Anim{};

        std::vector<RECT> m_StaticSolids;   // 정적 충돌(바닥/벽)
        POINTF m_Vel{ 0.f, 0.f };           // 플레이어 속도 (px/s)
        bool   m_Grounded = false;          // 지면 접촉 상태

        bool m_comInitialized = false; // CoInitializeEx 성공 여부
        bool m_isMoving = false;
        bool m_debugDrawEnabled = true;
    };

} // namespace engine
