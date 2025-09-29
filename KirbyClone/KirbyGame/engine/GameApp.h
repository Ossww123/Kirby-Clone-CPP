#pragma once
#include <windows.h>
#include <cwchar>
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "game/Player.h"
#include "engine/Math.h"  // Player.h에 있긴하지만 명시

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
            m_halfW = ( rc.right - rc.left ) / 2;
            m_halfH = ( rc.bottom - rc.top ) / 2;
            m_Player = m_Scene.Spawn<game::Player> ( rc );
        }

        // 윈도우 메시지 전달(휠/포커스 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        // 리사이즈 콜백 (경계 갱신)
        void OnResize ( int w , int h ) {
            m_halfW = w / 2; m_halfH = h / 2;

            if ( m_Player ) {
                RECT rc{ 0,0,w,h };
                m_Player->SetBounds ( rc );
            }
            EnsureBackBuffer ( );
        }

        // WM_PAINT에서 호출
        void OnPaint ( ) { Render ( ); }

        // 루프 1프레임 처리
        bool DoOneFrame ( ) {
            m_Time.TickFrame ( );
            m_Input.BeginFrame ( );

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
        }

        void FixedUpdate ( double fixedDt ) {
            m_Scene.Update ( fixedDt , m_Input );

            if ( m_Player ) {
                const engine::Vec2 c = m_Player->Center ( );
                const float k = 10.f; // 스무딩 강도
                m_camX += ( c.x - m_camX ) * static_cast< float >( k * fixedDt );
                m_camY += ( c.y - m_camY ) * static_cast< float >( k * fixedDt );
            }
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

            // 2) 씬 렌더 (메모리 DC에!)
            const int ox = static_cast< int >( m_camX ) - m_halfW;
            const int oy = static_cast< int >( m_camY ) - m_halfH;
            m_Scene.Render ( m_memDC , ox , oy );

            // 3) HUD 텍스트 (메모리 DC에!)
            SetBkMode ( m_memDC , TRANSPARENT );
            SetTextColor ( m_memDC , RGB ( 240 , 240 , 240 ) );
            wchar_t buf[ 256 ];
            std::swprintf ( buf , _countof ( buf ) ,
                L"FPS:%d  dt:%.3f  Cam(%.0f,%.0f)  MoveX:%.2f  Mouse(%d,%d) d(%d,%d) wheel:%d" ,
                m_Time.FPS ( ) , m_Time.FixedDelta ( ) , m_camX , m_camY , m_Input.GetAxis ( "MoveX" ) ,
                m_Input.MousePos ( ).x , m_Input.MousePos ( ).y , m_Input.MouseDelta ( ).x , m_Input.MouseDelta ( ).y ,
                m_Input.ConsumeWheel ( ) );
            TextOutW ( m_memDC , 8 , 8 , buf , lstrlenW ( buf ) );

            // 4) 한번에 화면으로 복사
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

        game::Player* m_Player{};

        // --- 백버퍼 ---
        HDC      m_memDC = nullptr;
        HBITMAP  m_backBMP = nullptr;
        HBITMAP  m_oldBMP = nullptr;
        int      m_bbW = 0 , m_bbH = 0;

        // --- 카메라 ---
        float m_camX = 0.f , m_camY = 0.f;
        int   m_halfW = 0 , m_halfH = 0;
    };

} // namespace engine
