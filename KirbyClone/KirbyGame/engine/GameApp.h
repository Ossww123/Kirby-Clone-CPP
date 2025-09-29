#pragma once
#include <windows.h>
#include <memory>
#include <cwchar>
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"
#include "engine/Camera.h"
#include "engine/IRenderer.h"
#include "engine/D3D11Renderer.h"
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
            const int w = rc.right - rc.left , h = rc.bottom - rc.top;

            // 씬/플레이어
            m_Player = m_Scene.Spawn<game::Player> ( rc );

            // 카메라
            m_Cam.SetScreenSize ( w , h );
            m_Cam.SetWorldRect ( 0.f , 0.f , 3000.f , 1600.f );
            m_Cam.SetSmoothSpeed ( 10.f ); m_Cam.SetPixelSnap ( true );
            m_Cam.SetLookAt ( m_Player->Center ( ) ); m_Cam.SnapImmediate ( );

            // D3D11 렌더러
            m_Renderer = std::make_unique<D3D11Renderer> ( );
            if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) {
                // 실패 시 앱 종료
                PostQuitMessage ( -1 );
            }
        }

        // 메시지 전달 (휠/포커스 등)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
            return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
        }

        void OnResize ( int w , int h ) {
            if ( m_Player ) {
                RECT rc{ 0,0,w,h };
                m_Player->SetBounds ( rc );
            }
            m_Cam.SetScreenSize ( w , h );
            if ( m_Renderer ) m_Renderer->Resize ( w , h );
        }

        // 루프 1 프레임
        bool DoOneFrame ( ) {
            m_Time.TickFrame ( );
            m_Input.BeginFrame ( );

            if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
                PostQuitMessage ( 0 );
                return false;
            }

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
            m_Input.BindAction ( "Quit" , VK_F10 );
            m_Input.BindAction ( "Jump" , VK_SPACE );
            m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
            m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
        }

        void FixedUpdate ( double fixedDt ) {
            m_Scene.Update ( fixedDt , m_Input );
            if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
            m_Cam.Update ( fixedDt );
        }

        void RenderFrame ( ) {
            // 당장은 배경색 클리어만: 다음 단계에서 스프라이트/디버그 드로우 이식
            m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } ); // 어두운 회색-남색 톤
            // (TODO) Sprite/Lines → D3D 버전으로 이식 후 여기서 호출
            m_Renderer->EndFrame ( );
        }

    private:
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        std::unique_ptr<IRenderer> m_Renderer;

        Camera m_Cam{};
        game::Player* m_Player{};
    };

} // namespace engine
