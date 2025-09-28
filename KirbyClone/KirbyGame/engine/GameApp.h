#pragma once
#include <windows.h>
#include <cwchar>
#include "Time.h"
#include "Input.h"

namespace engine {

    class GameApp {
    public:
        void Init ( HWND hWnd ) {
            m_hWnd = hWnd;
            m_Time.Init ( );
            m_Input.Init ( hWnd );
            InitBindings ( );
        }

        // 윈도우 메시지 전달(마우스 휠 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        // 프레임 단위 호출(메인 루프에서)
        bool DoOneFrame ( ) {
            m_Time.TickFrame ( );
            m_Input.BeginFrame ( );

            if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
                PostQuitMessage ( 0 );
                return false;
            }

            // 고정 업데이트(최대 스텝 제한)
            int steps = 0;
            constexpr int MAX_STEPS = 5;
            while ( m_Time.ShouldFixedUpdate ( ) && steps < MAX_STEPS ) {
                FixedUpdate ( m_Time.FixedDelta ( ) );
                m_Time.ConsumeFixedStep ( );
                ++steps;
            }

            RedrawWindow ( m_hWnd , nullptr , nullptr , RDW_INVALIDATE | RDW_UPDATENOW );

            return true;
        }

        // WM_PAINT에서 호출할 공개 함수
        void OnPaint ( ) { Render ( ); }

    private:
        void InitBindings ( ) {
            // === 액션 ===
            m_Input.BindAction ( "Quit" , VK_F10 );
            m_Input.BindAction ( "Jump" , VK_SPACE );
            m_Input.BindAction ( "Attack" , 'J' );     // 예시: J 공격
            m_Input.BindAction ( "Dash" , 'K' );     // 예시: K 대시

            // === 축 ===
            // MoveX: A(-1) / D(+1), ←(-1) / →(+1)
            m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
            m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );

            // MoveY: W(+1) / S(-1), ↑(+1) / ↓(-1)  (상하 반전은 게임 규칙에 맞게 조정)
            m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );

            // 마우스 X/Y 델타도 축에 기여하고 싶다면(선택):
            // m_Input.EnableMouseDeltaAsAxis("LookX", "LookY", 0.1f); // 예시
        }

        void FixedUpdate ( double fixedDt ) {
            // 여기서 m_Input 액션/축 사용해서 논리 업데이트
            // 예시: 축 읽기
            const float mx = m_Input.GetAxis ( "MoveX" );
            const float my = m_Input.GetAxis ( "MoveY" );
            ( void ) fixedDt; ( void ) mx; ( void ) my;
        }

        void Render ( ) {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint ( m_hWnd , &ps );

            RECT rc; GetClientRect ( m_hWnd , &rc );
            HBRUSH bg = CreateSolidBrush ( RGB ( 24 , 28 , 32 ) );
            FillRect ( hdc , &rc , bg );
            DeleteObject ( bg );

            SetBkMode ( hdc , TRANSPARENT );
            SetTextColor ( hdc , RGB ( 240 , 240 , 240 ) );

            wchar_t buf[ 256 ];
            std::swprintf ( buf , _countof ( buf ) ,
                L"FPS:%d | dt(fixed):%.3f | Jump:%s | MoveX:%.2f | Mouse(%d,%d) d(%d,%d) wheel:%d" ,
                m_Time.FPS ( ) , m_Time.FixedDelta ( ) ,
                m_Input.ActionDown ( "Jump" ) ? L"Down" : L"Up" ,
                m_Input.GetAxis ( "MoveX" ) ,
                m_Input.MousePos ( ).x , m_Input.MousePos ( ).y ,
                m_Input.MouseDelta ( ).x , m_Input.MouseDelta ( ).y ,
                m_Input.ConsumeWheel ( ) ); // 표시에 쓴 뒤 0으로 리셋

            TextOutW ( hdc , 8 , 8 , buf , lstrlenW ( buf ) );
            EndPaint ( m_hWnd , &ps );
        }

    private:
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
    };

} // namespace engine
