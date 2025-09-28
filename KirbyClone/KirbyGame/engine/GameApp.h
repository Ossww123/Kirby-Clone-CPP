#pragma once
#include <windows.h>
#include <cwchar>
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "game/Player.h"

namespace engine {

    class GameApp {
    public:
        void Init ( HWND hWnd ) {
            m_hWnd = hWnd;
            m_Time.Init ( );
            m_Input.Init ( hWnd );
            InitBindings ( );

            RECT rc; GetClientRect ( m_hWnd , &rc );
            m_Player = m_Scene.Spawn<game::Player> ( rc );
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
            RedrawWindow ( m_hWnd , nullptr , nullptr , RDW_INVALIDATE | RDW_UPDATENOW );
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
        }

        void Render ( ) {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint ( m_hWnd , &ps );

            RECT rc; GetClientRect ( m_hWnd , &rc );
            HBRUSH bg = CreateSolidBrush ( RGB ( 24 , 28 , 32 ) );
            FillRect ( hdc , &rc , bg );
            DeleteObject ( bg );

            // 씬 렌더
            m_Scene.Render ( hdc );

            // HUD 텍스트
            SetBkMode ( hdc , TRANSPARENT );
            SetTextColor ( hdc , RGB ( 240 , 240 , 240 ) );
            wchar_t buf[ 256 ];
            std::swprintf ( buf , _countof ( buf ) ,
                L"FPS:%d  dt(fixed):%.3f  MoveX:%.2f  Mouse(%d,%d) d(%d,%d) wheel:%d" ,
                m_Time.FPS ( ) , m_Time.FixedDelta ( ) ,
                m_Input.GetAxis ( "MoveX" ) ,
                m_Input.MousePos ( ).x , m_Input.MousePos ( ).y ,
                m_Input.MouseDelta ( ).x , m_Input.MouseDelta ( ).y ,
                m_Input.ConsumeWheel ( ) );
            TextOutW ( hdc , 8 , 8 , buf , lstrlenW ( buf ) );

            EndPaint ( m_hWnd , &ps );
        }

    private:
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};
        game::Player* m_Player{};
    };

} // namespace engine
