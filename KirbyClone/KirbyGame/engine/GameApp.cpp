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
        engine::TileDef solid{};  solid.solid = true;  solid.src = RECT{ 0,0,32,32 };
        engine::TileDef oneway{}; oneway.oneway = true;  oneway.src = RECT{ 32,0,64,32 };
        m_World.DefineTile ( 1 , solid );
        m_World.DefineTile ( 2 , oneway );

        // 맵 로드 + 콜라이더
        if ( m_World.LoadMapCSV ( L"assets/stage01.csv" ) ) {
            m_World.RebuildColliders ( );

            // 카메라 월드 사각형 자동 설정
            RECT wr = m_World.WorldRectPx ( );
            m_Cam.SetWorldRect ( ( float ) wr.left , ( float ) wr.top , ( float ) wr.right , ( float ) wr.bottom );
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

        // ⇩ 한 줄로 교체
        m_PlayerFSM.Step ( fixedDt , m_Input );

        // (선택) 외부 애니메이터를 아직 쓰고 있다면, 프레임 진행만 남겨도 됨
        // m_Anim.Update(fixedDt);

        // 카메라만 유지
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.Update ( fixedDt );
    }


    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        // BeginFrame은 Color 타입을 받도록 수정
        Color clear{ 0.09f, 0.11f, 0.125f, 1.0f };
        m_Renderer->BeginFrame ( clear );

        auto [ox , oy] = m_Cam.OffsetInt ( );

        // --- SpriteBatch ---
        if ( m_Batch ) {
            m_Batch->Begin ( );

            // 1) 타일맵 (가시 영역만)
            m_World.RenderVisible ( *m_Batch , ox , oy , d3d->Width ( ) , d3d->Height ( ) );

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
            const int wx0 = ox , wy0 = oy , wx1 = ox + d3d->Width ( ) , wy1 = oy + d3d->Height ( );
            int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
            for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
            for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );

            // 병합된 SOLID 콜라이더
            m_World.Collision ( ).DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) );

            if ( m_Player ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
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

            wchar_t st[ 64 ];
            std::swprintf ( st , _countof ( st ) , L"STATE: %S" , m_PlayerFSM.StateName ( ) );
            m_TextHUD->DrawTextLine ( st , 8.f , 28.f );

            m_TextHUD->End ( );
        }
        m_Renderer->EndFrame ( );
    }


} // namespace engine
