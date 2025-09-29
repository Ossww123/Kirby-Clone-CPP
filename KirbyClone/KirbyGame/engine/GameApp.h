#pragma once
#include <windows.h>
#include <cwchar>
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "game/Player.h"
#include "engine/Camera.h"
#include "engine/Math.h"  // Player.h에 있긴하지만 명시
#include "engine/DebugDraw.h"

namespace engine {

    class GameApp {
    public:
        ~GameApp ( ) { CleanupBackBuffer ( ); }

        void Init ( HWND hWnd ) {
            m_hWnd = hWnd;
            m_Time.Init ( );
            m_Input.Init ( hWnd );
            InitBindings ( );

            RECT rc; GetClientRect ( m_hWnd , &rc );
            m_Player = m_Scene.Spawn<game::Player> ( rc );

            const int w = rc.right - rc.left , h = rc.bottom - rc.top;
            m_Cam.SetScreenSize ( w , h );
            m_Cam.SetWorldRect ( 0.f , 0.f , 3000.f , 1600.f ); // 데모용 월드 크기
            m_Cam.SetSmoothSpeed ( 10.f );
            m_Cam.SetPixelSnap ( true );
            m_Cam.SetLookAt ( m_Player->Center ( ) );
            m_Cam.SnapImmediate ( );
        }

        // 윈도우 메시지 전달(휠/포커스 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        // 리사이즈 콜백 (경계 갱신)
        void OnResize ( int w , int h ) {
            if ( m_Player ) {
                RECT rc{ 0,0,w,h };
                m_Player->SetBounds ( rc );
            }
            m_Cam.SetScreenSize ( w , h );
            EnsureBackBuffer ( );
        }

        // WM_PAINT에서 호출
        void OnPaint ( ) { Render ( ); }

        // 루프 1프레임 처리
        bool DoOneFrame ( ) {
            m_Time.TickFrame ( );
            m_Input.BeginFrame ( );
            engine::debug::BeginFrame ( );

            if ( m_Input.ActionPressed ( "ToggleDebug" ) )   // F1 토글
                m_debugDrawEnabled = !m_debugDrawEnabled;

            if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
                PostQuitMessage ( 0 );
                return false;
            }

            // 고정 업데이트
            int steps = 0;
            constexpr int MAX_STEPS = 5;
            while ( m_Time.ShouldFixedUpdate ( ) && steps < MAX_STEPS ) {
                FixedUpdate ( m_Time.FixedDelta ( ) );
                m_Time.ConsumeFixedStep ( );
                ++steps;
            }

            // 이번 프레임 즉시 페인트
            RedrawWindow ( m_hWnd , nullptr , nullptr ,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE );

            return true;
        }

    private:
        void InitBindings ( ) {
            // 액션
            m_Input.BindAction ( "Quit" , VK_F10 );
            m_Input.BindAction ( "Jump" , VK_SPACE );
            m_Input.BindAction ( "Attack" , 'J' );
            m_Input.BindAction ( "Dash" , 'K' );

            // 축
            m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
            m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );

            // 디버그
            m_Input.BindAction ( "ToggleDebug" , VK_F1 );
        }

        void FixedUpdate ( double fixedDt ) {
            m_Scene.Update ( fixedDt , m_Input );

            if ( m_Player ) {
                m_Cam.SetLookAt ( m_Player->Center ( ) );
            }
            m_Cam.Update ( fixedDt );
        }

        void Render ( ) {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint ( m_hWnd , &ps );

            EnsureBackBuffer ( );

            // 1) 백버퍼를 배경색으로 지우기
            HBRUSH bg = CreateSolidBrush ( RGB ( 24 , 28 , 32 ) );
            RECT rc{ 0,0,m_bbW,m_bbH };
            FillRect ( m_memDC , &rc , bg );
            DeleteObject ( bg );

            // 2) 씬 렌더 (메모리 DC에)
            auto [ox , oy] = m_Cam.OffsetInt ( );
            m_Scene.Render ( m_memDC , ox , oy );

            // 2.5) 디버그 드로우 수집
            if ( m_debugDrawEnabled ) {
                // (i) 그리드 (32px 간격)
                const int GRID = 32;
                const int wx0 = ox , wy0 = oy;
                const int wx1 = ox + m_bbW , wy1 = oy + m_bbH;

                int gxStart = ( wx0 / GRID ) * GRID;
                int gyStart = ( wy0 / GRID ) * GRID;

                for ( int x = gxStart; x <= wx1; x += GRID ) {
                    engine::debug::WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
                }
                for ( int y = gyStart; y <= wy1; y += GRID ) {
                    engine::debug::WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );
                }

                // (ii) 플레이어 AABB
                if ( m_Player ) {
                    int px , py , pw , ph;
                    m_Player->GetBounds ( px , py , pw , ph );
                    engine::debug::WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) );
                }

                // (iii) 화면 중심 표식
                const int cx = m_bbW / 2 , cy = m_bbH / 2;
                engine::debug::Line ( cx - 6 , cy , cx + 6 , cy , RGB ( 200 , 200 , 80 ) );
                engine::debug::Line ( cx , cy - 6 , cx , cy + 6 , RGB ( 200 , 200 , 80 ) );
            }

            // 3) 디버그 커맨드 플러시 (메모리 DC 대상으로)
            engine::debug::Flush ( m_memDC );

            // 4) HUD
            SetBkMode ( m_memDC , TRANSPARENT );
            SetTextColor ( m_memDC , RGB ( 240 , 240 , 240 ) );
            wchar_t buf[ 256 ];
            std::swprintf ( buf , _countof ( buf ) ,
                L"FPS:%d  dt:%.3f  Debug:%s" ,
                m_Time.FPS ( ) , m_Time.FixedDelta ( ) ,
                m_debugDrawEnabled ? L"ON" : L"OFF" );
            TextOutW ( m_memDC , 8 , 8 , buf , lstrlenW ( buf ) );

            // 5) 백버퍼 → 화면
            BitBlt ( hdc , 0 , 0 , m_bbW , m_bbH , m_memDC , 0 , 0 , SRCCOPY );
            EndPaint ( m_hWnd , &ps );
        }

        // --- 더블 버퍼링 ---
        void EnsureBackBuffer ( ) {
            RECT rc; GetClientRect ( m_hWnd , &rc );
            const int w = rc.right - rc.left;
            const int h = rc.bottom - rc.top;
            if ( w <= 0 || h <= 0 ) return;

            if ( m_memDC && w == m_bbW && h == m_bbH ) return; // 크기 동일 → 재사용

            HDC wndDC = GetDC ( m_hWnd );
            if ( !m_memDC ) m_memDC = CreateCompatibleDC ( wndDC );

            // 기존 비트맵 해제
            if ( m_backBMP ) {
                SelectObject ( m_memDC , m_oldBMP );
                DeleteObject ( m_backBMP );
                m_backBMP = nullptr;
            }

            m_backBMP = CreateCompatibleBitmap ( wndDC , w , h );
            m_oldBMP = ( HBITMAP ) SelectObject ( m_memDC , m_backBMP );
            ReleaseDC ( m_hWnd , wndDC );

            m_bbW = w; m_bbH = h;
        }

        void CleanupBackBuffer ( ) {
            if ( m_memDC ) {
                if ( m_backBMP ) {
                    SelectObject ( m_memDC , m_oldBMP );
                    DeleteObject ( m_backBMP );
                    m_backBMP = nullptr;
                }
                DeleteDC ( m_memDC );
                m_memDC = nullptr;
            }
            m_oldBMP = nullptr;
            m_bbW = m_bbH = 0;
        }

    private:
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};
        Camera m_Cam{};

        game::Player* m_Player{};

        // --- 백버퍼 ---
        HDC      m_memDC = nullptr;
        HBITMAP  m_backBMP = nullptr;
        HBITMAP  m_oldBMP = nullptr;
        int      m_bbW = 0 , m_bbH = 0;

        // --- 디버깅 ---
        bool  m_debugDrawEnabled = true;
    };

} // namespace engine
