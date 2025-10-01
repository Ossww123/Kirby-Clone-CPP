#pragma once
#include <windows.h>
#include <memory>
#include <cwchar>
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"
#include "engine/Camera.h"
#include "engine/Anim.h"
#include "engine/IRenderer.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11Sprite.h"
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

            // D3D11 렌더러 생성
            m_Renderer = std::make_unique<D3D11Renderer> ( );
            if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) {
                PostQuitMessage ( -1 );
                return;
            }

            // 스프라이트 렌더러 초기화
            auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );
            m_Sprites = std::make_unique<D3D11SpriteRenderer> ( );
            m_Sprites->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

            // WIC 초기화 + 텍스처 로드
            HRESULT cohr = CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
            if ( SUCCEEDED ( cohr ) ) m_comInitialized = true; // 이미 초기화되어 있으면 S_FALSE
            // 실행 디렉터리 기준 경로 (VS의 작업 디렉터리를 프로젝트 루트로 맞추는 것을 권장)
            if ( !LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex ) ) {
                // TODO: 플레이스홀더 생성이나 오류 로그를 원하면 여기에 처리
            }

            // 텍스처 로드 후 (성공 시)
            if ( m_PlayerTex.srv ) {
                // 1) Idle: 전체 이미지 1프레임
                RECT full{ 0, 0, m_PlayerTex.width, m_PlayerTex.height };
                engine::AnimClip idle{};
                idle.frames.push_back ( { full, 0.2f } );
                idle.loop = true;
                m_Anim.AddClip ( "Idle" , std::move ( idle ) );

                // 2) Walk: 시트가 있다면 행 기반 프레임(예: 32x32 셀, 6프레임, 12fps)
                // → 실제 시트에 맞춰 값만 바꿔주면 됨.
                // engine::AnimClip walk = engine::Animator::MakeRowClip(0, 0, 32, 32, 6, 12.f, true);
                // m_Anim.AddClip("Walk", std::move(walk));

                // 일단 Idle로 시작
                m_Anim.Play ( "Idle" , true );

                // 크기 맞추기(원본 비율 유지)
                if ( m_Player ) m_Player->SetSize ( ( float ) m_PlayerTex.width , ( float ) m_PlayerTex.height );
            }

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
            if ( m_Sprites )  m_Sprites->OnResize ( w , h );
        }

        // 루프 1 프레임
        bool DoOneFrame ( ) {
            m_Time.TickFrame ( );
            m_Input.BeginFrame ( );

            if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
                PostQuitMessage ( 0 );
                return false;
            }

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

            // 축
            m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
            m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
            m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
        }

        void FixedUpdate ( double fixedDt ) {
            m_Scene.Update ( fixedDt , m_Input );

            // 이동 판단 (입력 기준; 플레이어 내부보다 여기서 간단히 판단)
            const float mx = m_Input.GetAxis ( "MoveX" );
            const float my = m_Input.GetAxis ( "MoveY" );
            m_isMoving = ( std::fabs ( mx ) > 0.05f ) || ( std::fabs ( my ) > 0.05f );

            // 클립 전환 (Walk 클립이 없으면 Idle 유지)
            if ( m_isMoving && m_Anim.CurrentName ( ) != "Walk" ) {
                if ( !m_Anim.Play ( "Walk" , false ) ) m_Anim.Play ( "Idle" , false );
            }
            else if ( !m_isMoving && m_Anim.CurrentName ( ) != "Idle" ) {
                m_Anim.Play ( "Idle" , false );
            }

            m_Anim.Update ( fixedDt );

            if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
            m_Cam.Update ( fixedDt );
        }


        void RenderFrame ( ) {
            m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );

            if ( m_Player && m_PlayerTex.srv && m_Sprites ) {
                auto [ox , oy] = m_Cam.OffsetInt ( );
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );

                const float x = float ( px - ox ) , y = float ( py - oy );
                const float w = float ( pw ) , h = float ( ph );

                RECT src = m_Anim.CurrentSrc ( );
                float tint[ 4 ] = { 1,1,1,1 };
                m_Sprites->Draw ( m_PlayerTex , x , y , w , h , &src , tint );
            }

            m_Renderer->EndFrame ( );
        }


    private:
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        std::unique_ptr<IRenderer>            m_Renderer;
        std::unique_ptr<D3D11SpriteRenderer>  m_Sprites;
        Tex2D                                  m_PlayerTex{};

        Camera          m_Cam{};
        game::Player* m_Player{};
        engine::Animator m_Anim{};

        bool m_comInitialized = false; // CoInitializeEx 성공 여부
        bool m_isMoving = false;
    };

} // namespace engine
