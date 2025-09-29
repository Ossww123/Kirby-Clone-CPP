#include <windows.h>
#include "engine/GameApp.h"

static int   gClientW = 960;
static int   gClientH = 540;
static engine::GameApp gApp;

LRESULT CALLBACK WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
    gApp.OnWndMessage ( hWnd , msg , wParam , lParam );

    switch ( msg )
    {
    case WM_SIZE: {
        const int w = LOWORD ( lParam );
        const int h = HIWORD ( lParam );
        gApp.OnResize ( w , h );
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // OS 배경 지우기 방지
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint ( hWnd , &ps );
        EndPaint ( hWnd , &ps );
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage ( 0 );
        return 0;
    }
    return DefWindowProcW ( hWnd , msg , wParam , lParam );
}


int WINAPI wWinMain ( HINSTANCE hInst , HINSTANCE , PWSTR , int nCmdShow )
{
    const wchar_t* kClass = L"KirbyGameWindowClass";
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor ( nullptr , IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszClassName = kClass;
    RegisterClassW ( &wc );

    RECT r{ 0,0,gClientW,gClientH };
    AdjustWindowRect ( &r , WS_OVERLAPPEDWINDOW , FALSE );
    HWND hWnd = CreateWindowExW (
        0 , kClass , L"KirbyGame" ,
        WS_OVERLAPPEDWINDOW , CW_USEDEFAULT , CW_USEDEFAULT ,
        r.right - r.left , r.bottom - r.top ,
        nullptr , nullptr , hInst , nullptr );

    ShowWindow ( hWnd , nCmdShow );
    UpdateWindow ( hWnd );

    gApp.Init ( hWnd );

    MSG msg{};
    bool running = true;
    while ( running )
    {
        while ( PeekMessageW ( &msg , nullptr , 0 , 0 , PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) { running = false; break; }
            TranslateMessage ( &msg );
            DispatchMessageW ( &msg );
        }
        if ( !running ) break;

        // 한 프레임 처리
        if ( !gApp.DoOneFrame ( ) ) break;
    }
    return 0;
}
