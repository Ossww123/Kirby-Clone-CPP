#include "engine/core/GameApp.h"

// Win32 / COM
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>

// std
#include <algorithm>

// engine core
#include "engine/core/Time.h"
#include "engine/core/Input.h"
#include "engine/core/Scene.h"

// renderer pieces
#include "engine/render/IRenderer.h"
#include "engine/render/D3D11Renderer.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/render/D3D11DebugDraw.h"
#include "engine/render/DWriteText.h"

#include "engine/platform/win32/RectUtil.h"

// game
#include "game/session/PlaySession.h"

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
        InitBindings ( );

        // window size
        RECT rc{}; ::GetClientRect ( m_hWnd , &rc );
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;

        InitRendererUI ( hWnd , w , h );

        // COM for WIC/DirectWrite users (idempotent)
        if ( !m_comInitialized ) {
            const HRESULT cohr = ::CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
            if ( SUCCEEDED ( cohr ) ) m_comInitialized = true;
        }

        // Create/boot session
        m_Session = std::make_unique<game::PlaySession> ( );
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        ( void ) d3d; // may be unused in release
        m_Session->Initialize ( { m_Renderer.get ( ), m_Batch.get ( ), m_Debug.get ( ), m_TextHUD.get ( ), &m_Scene, engine::win32::FromRECT(rc) } );
        m_Session->LoadStage ( "assets/stages/stage01/stage.json" );
    }

    LRESULT GameApp::OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
    {
        return m_Input.OnWndMessage ( hWnd , msg , wParam , lParam );
    }

    void GameApp::OnResize ( int w , int h )
    {
        if ( w <= 0 || h <= 0 ) return;

        // 1) resize swapchain/RTV first
        if ( m_Renderer ) m_Renderer->Resize ( w , h );

        // 2) query the actual backbuffer size (single source of truth)
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : w;
        const int sh = d3d ? d3d->Height ( ) : h;
        if ( sw <= 0 || sh <= 0 ) return;

        // 3) propagate to render helpers & game
        m_Render.OnResize ( sw , sh );
        if ( m_Session ) m_Session->OnResize ( sw , sh );
    }

    bool GameApp::DoOneFrame ( )
    {
        m_Time.TickFrame ( );
        m_Input.BeginFrame ( );

        if ( m_Input.ActionPressed ( "Quit" ) || m_Input.Pressed ( VK_ESCAPE ) ) {
            ::PostQuitMessage ( 0 );
            return false;
        }
        if ( m_Input.ActionPressed ( "ToggleDebug" ) ) m_debugDrawEnabled = !m_debugDrawEnabled;
        if ( m_Input.ActionPressed ( "Reload" ) && m_Session ) {
            m_Session->ReloadStage ( );
        }

        m_Time.CapAccumulator ( 5 );
        while ( m_Time.ShouldFixedUpdate ( ) ) {
            FixedUpdate ( m_Time.FixedDelta ( ) );
            m_Time.ConsumeFixedStep ( );
        }

        RenderFrame ( );
        return true;
    }

    void GameApp::InitBindings ( )
    {
        // Actions
        m_Input.BindAction ( "Quit" , VK_F10 );
        m_Input.BindAction ( "Jump" , 'Z' );
        m_Input.BindAction ( "Attack" , 'X' );
        m_Input.BindAction ( "Interact" , VK_UP );
        m_Input.BindAction ( "ToggleDebug" , VK_F1 );
        m_Input.BindAction ( "Reload" , VK_F5 );

        // Axes
        m_Input.BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
        m_Input.BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
        m_Input.BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
    }

    void GameApp::FixedUpdate ( double fixedDt )
    {
        if ( m_Session ) m_Session->FixedUpdate ( fixedDt , m_Input );
    }

    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : 0;
        const int sh = d3d ? d3d->Height ( ) : 0;

        // frame begin
        m_Renderer->BeginFrame ( { 0.09f, 0.11f, 0.125f, 1.0f } );

        const auto [ox , oy] = m_Session ? m_Session->CameraOffsetInt ( ) : std::pair<int , int>{ 0,0 };

        if ( m_Batch ) {
            m_Batch->Begin ( );
            if ( m_Session ) {
                m_Session->RenderParallaxBG ( ox , oy , sw , sh );
                m_Session->RenderWorld ( ox , oy , sw , sh );
                m_Session->RenderOverlayFade ( sw , sh );
            }
            m_Batch->End ( );
        }

        if ( m_Session )
            m_Session->RenderDebugGridAndColliders ( ox , oy , sw , sh , m_debugDrawEnabled );

        if ( m_Session )
            m_Session->RenderHUD ( m_Time.FPS ( ) , m_Time.FixedDelta ( ) );

        // frame end
        m_Renderer->EndFrame ( );
    }

    void GameApp::InitRendererUI ( HWND hWnd , int w , int h )
    {
        // renderer
        m_Renderer = std::make_unique<D3D11Renderer> ( );
        if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) {
            ::PostQuitMessage ( -1 );
            return;
        }
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        // sprite batch
        m_Batch = std::make_unique<engine::D3D11SpriteBatch> ( );
        m_Batch->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );

        // HUD text
        m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
        m_TextHUD->Initialize ( d3d->SwapChain ( ) );

        // debug draw
        m_Debug = std::make_unique<engine::D3D11DebugDraw> ( );
        m_Debug->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) );
    }

} // namespace engine
