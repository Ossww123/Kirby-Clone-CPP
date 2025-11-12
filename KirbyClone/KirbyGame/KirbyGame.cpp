//
// KirbyGame.cpp
//
// Responsibility: Win32 bootstrap — register/create the window, bridge window messages to GameApp,
//                 and run the main loop.
// Non-Goals: Engine subsystems, rendering details, or gameplay logic (handled by GameApp/engine).
// Call-Context: Windows desktop app (UTF-16 entry point; single-threaded main loop).
//

#include <windows.h>
#include <cstdint>
#include "engine/GameApp.h"
#include "game/session/PlaySession.h"
#include "game/frontend/FrontFlow.h"
#include "game/GameConfig.h"
#include "engine/core/Time.h"
#include "engine/core/Input.h"
#include "engine/core/Scene.h"
#include "engine/render/IRenderer.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/render/D3D11DebugDraw.h"
#include "engine/render/DWriteText.h"

static int gClientW = game::CLIENT_W;  // 240*4 = 960
static int gClientH = game::CLIENT_H;  // 160*4 = 640
static engine::GameApp gApp;

// Win32 window procedure: dispatch messages and hand off what we care about to GameApp/Input.
LRESULT CALLBACK WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
    // 1) Bridge to GameApp (cast to platform-agnostic signature).
    const auto r = gApp.OnWndMessage (
        hWnd ,
        static_cast< unsigned >( msg ) ,
        static_cast< std::uintptr_t >( wParam ) ,
        static_cast< std::intptr_t >( lParam )
    );
    // If GameApp/Input handled it, return immediately.
    if ( r != 0 ) return static_cast< LRESULT >( r );

    switch ( msg )
    {
    case WM_SIZE: {
        // Client size changed — forward to GameApp resize.
        const int w = LOWORD ( lParam );
        const int h = HIWORD ( lParam );
        gApp.OnResize ( w , h );
        return 0;
    }
    case WM_ERASEBKGND:
        // Prevent flicker by telling the OS we handled background erase.
        return 1;

    case WM_PAINT: {
        // Validate the invalid region; we render on our own cadence.
        PAINTSTRUCT ps;
        BeginPaint ( hWnd , &ps );
        EndPaint ( hWnd , &ps );
        return 0;
    }
    case WM_DESTROY:
        // Post quit to exit the message loop.
        PostQuitMessage ( 0 );
        return 0;
    }
    // Default handling for anything we don't care about.
    return DefWindowProcW ( hWnd , msg , wParam , lParam );
}

int WINAPI wWinMain ( HINSTANCE hInst , HINSTANCE , PWSTR , int nCmdShow )
{
    const wchar_t* kClass = L"KirbyGameWindowClass";

    // Register a minimal window class.
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;                 // Dedicated DC per window.
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor ( nullptr , IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszClassName = kClass;
    RegisterClassW ( &wc );

    // Compute outer rect for desired client size.
    RECT r{ 0, 0, gClientW, gClientH };
    AdjustWindowRect ( &r , WS_OVERLAPPEDWINDOW , FALSE );

    // Create the window.
    HWND hWnd = CreateWindowExW (
        0 ,
        kClass ,
        L"KirbyGame" ,
        WS_OVERLAPPEDWINDOW ,
        CW_USEDEFAULT ,
        CW_USEDEFAULT ,
        r.right - r.left ,
        r.bottom - r.top ,
        nullptr ,
        nullptr ,
        hInst ,
        nullptr );

    ShowWindow ( hWnd , nCmdShow );
    UpdateWindow ( hWnd );

    // Engine bootstrap.
    gApp.Init ( hWnd );

    // Standard message pump + game loop.
    MSG msg{};
    bool running = true;
    while ( running )
    {
        // Drain all pending messages without blocking.
        while ( PeekMessageW ( &msg , nullptr , 0 , 0 , PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) { running = false; break; }
            TranslateMessage ( &msg );
            DispatchMessageW ( &msg );
        }
        if ( !running ) break;

        // Advance one frame; break to shutdown on false.
        if ( !gApp.DoOneFrame ( ) ) break;
    }
    return 0;
}
