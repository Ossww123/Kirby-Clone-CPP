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
#include "game/frontend/FrontFlow.h"
#include "game/data/StagePath.h" // StageJsonPathFromId

#ifndef DBGLOG
#include <string>
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) { ::OutputDebugStringW ( msg ); ::OutputDebugStringW ( L"\n" ); }
#endif

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
        // pointer-based ownership
        m_Time = std::make_unique<Time> ( );   m_Time->Init ( );
        m_Input = std::make_unique<Input> ( ); m_Input->Init ( hWnd );
        m_Scene = std::make_unique<Scene> ( );
        InitBindings ( );

        // COM for WIC/DirectWrite users (idempotent)
        if ( !m_comInitialized ) {
            const HRESULT cohr = ::CoInitializeEx ( nullptr , COINIT_MULTITHREADED );
            DBGLOG ( SUCCEEDED ( cohr ) ? L"[Init] CoInitializeEx OK" : L"[Init] CoInitializeEx FAIL" );
            if ( SUCCEEDED ( cohr ) ) m_comInitialized = true;
        }

        // window size
        RECT rc{}; ::GetClientRect ( m_hWnd , &rc );
        const int w = rc.right - rc.left, h = rc.bottom - rc.top;
        InitRendererUI ( hWnd , w , h );
        DBGLOG ( L"[Init] InitRendererUI done" );

        // === FrontFlow 부팅 ===
        m_mode = AppMode::Front;
        m_Front = std::make_unique<game::FrontFlow> ( );
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : w;
        const int sh = d3d ? d3d->Height ( ) : h;
        m_Front->Initialize ( {
            m_TextHUD.get ( ), m_Input.get ( ),& m_Save,& m_State, sw, sh
        } );

        DBGLOG ( L"[Init] FrontFlow initialized" );

        // Front→Session convert callback
        m_Front->onStartSolo = [ this ] ( int slot ) {
            // 1) save slot + load disk
            m_State.SetActiveSlot ( slot );
            ( void ) m_State.LoadFromDisk ( );
            const auto& sd = m_State.Data ( );

            // 2) Stage ID -> file path
            const std::string hubId = ( sd.lastHub.empty ( ) ? std::string ( "t1/hub" ) : sd.lastHub );
            const std::string stageJson = game::StageJsonPathFromId ( hubId );

            // 3) PlaySession init + load hub
            m_Session = std::make_unique<game::PlaySession> ( );
            auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
            RECT rc{}; ::GetClientRect ( m_hWnd , &rc );
            m_Session->Initialize ( {
                m_Renderer.get ( ),& m_Render, m_TextHUD.get ( ),
                m_Scene.get ( ), engine::win32::FromRECT ( rc ), & m_State
            } );
            m_Session->LoadStage ( stageJson.c_str ( ) );

            // 4) convert mode
            m_mode = AppMode::Session;
            m_Front.reset ( );
            };

    }

    std::intptr_t GameApp::OnWndMessage ( HWND hWnd , unsigned msg , std::uintptr_t wParam , std::intptr_t lParam )
    {
        return m_Input ? m_Input->OnWndMessage ( hWnd , msg , wParam , lParam ) : 0;
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

        if ( m_TextHUD ) m_TextHUD->Initialize ( d3d->SwapChain ( ) );

        if ( m_mode == AppMode::Front && m_Front ) m_Front->OnResize ( sw , sh );
        if ( m_mode == AppMode::Session && m_Session ) m_Session->OnResize ( sw , sh );
    }

    bool GameApp::DoOneFrame ( )
    {
        m_Time->TickFrame ( );
        m_Input->BeginFrame ( );

        if ( m_Input->ActionPressed ( "Quit" ) || m_Input->Pressed ( VK_ESCAPE ) ) {
            ::PostQuitMessage ( 0 );
            return false;
        }
        if ( m_Input->ActionPressed ( "ToggleDebug" ) ) m_debugDrawEnabled = !m_debugDrawEnabled;
        if ( m_Input->ActionPressed ( "Reload" ) && m_Session ) {
            m_Session->ReloadStage ( );
        }

        m_Time->CapAccumulator ( 5 );
        while ( m_Time->ShouldFixedUpdate ( ) ) {
            FixedUpdate ( m_Time->FixedDelta ( ) );
            m_Time->ConsumeFixedStep ( );
        }

        DBGLOG ( L"[DoOneFrame] RenderFrame about to be called" );
        RenderFrame ( );
        DBGLOG ( L"[DoOneFrame] RenderFrame returned" );
        return true;
    }

    void GameApp::InitBindings ( )
    {
        // Actions
        m_Input->BindAction ( "Quit" , VK_F10 );
        m_Input->BindAction ( "Jump" , 'Z' );
        m_Input->BindAction ( "Attack" , 'X' );
        m_Input->BindAction ( "Interact" , VK_UP );
        m_Input->BindAction ( "ToggleDebug" , VK_F1 );
        m_Input->BindAction ( "Reload" , VK_F5 );

        // === Confirm/Back for FrontFlow ===
        m_Input->BindAction ( "Confirm" , VK_RETURN );
        m_Input->BindAction ( "Confirm" , VK_SPACE );
        m_Input->BindAction ( "Back" , VK_BACK );

        // Axes
        m_Input->BindAxis ( "MoveX" , { .positiveVK = VK_RIGHT, .negativeVK = VK_LEFT, .scale = 1.f } );
        m_Input->BindAxis ( "MoveX" , { .positiveVK = 'D',      .negativeVK = 'A',     .scale = 1.f } );
        m_Input->BindAxis ( "MoveY" , { .positiveVK = VK_UP,    .negativeVK = VK_DOWN, .scale = 1.f } );
        m_Input->BindAxis ( "MoveY" , { .positiveVK = 'W',      .negativeVK = 'S',     .scale = 1.f } );
    }

    void GameApp::FixedUpdate ( double fixedDt ) {
        if ( m_mode == AppMode::Front ) {
            if ( m_Front ) m_Front->Update ( fixedDt );
            return;
        }
        if ( m_Session ) m_Session->FixedUpdate ( fixedDt , *m_Input );
    }

    void GameApp::RenderFrame ( )
    {
        auto* d3d = static_cast< engine::D3D11Renderer* >( m_Renderer.get ( ) );
        const int sw = d3d ? d3d->Width ( ) : 0;
        const int sh = d3d ? d3d->Height ( ) : 0;

        m_Render.Begin ( { 0.05f, 0.00f, 0.10f, 1.0f } );

        if ( m_mode == AppMode::Front && m_Front ) {
            m_Front->Render ( );
        }
        else if ( m_Session ) {
            const auto [ox , oy] = m_Session->CameraOffsetInt ( );
            m_Session->RenderParallaxBG ( ox , oy , sw , sh );
            m_Session->RenderWorld ( ox , oy , sw , sh );
            m_Session->RenderOverlayFade ( sw , sh );
            m_Session->RenderDebugGridAndColliders ( ox , oy , sw , sh , m_debugDrawEnabled );
            m_Session->RenderHUD ( m_Time->FPS ( ) , m_Time->FixedDelta ( ) ); // DWrite
        }

        m_Render.End ( );
    }


    void GameApp::InitRendererUI ( HWND hWnd , int w , int h )
    {
        // 1) renderer
        m_Renderer = std::make_unique<D3D11Renderer> ( );
        if ( !m_Renderer->Initialize ( hWnd , w , h , /*vsync=*/false ) ) { ::PostQuitMessage ( -1 ); return; }
        auto* d3d = static_cast< D3D11Renderer* >( m_Renderer.get ( ) );

        // 2) HUD text (DWrite)
        m_TextHUD = std::make_unique<engine::DWriteTextHUD> ( );
        m_TextHUD->Initialize ( d3d->SwapChain ( ) );

        // 3) RenderSystem (여기서 내부 batch/debug를 생성함)
        m_Render.Init ( m_Renderer.get ( ) );
        m_Render.OnResize ( w , h );

        DBGLOG ( L"[InitRendererUI] TextHUD Initialize OK" );
    }

} // namespace engine
