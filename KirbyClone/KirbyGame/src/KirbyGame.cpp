#include <windows.h>
#include <stdint.h>
#include <cwchar>

namespace core
{
    // 고정 업데이트 1/60초
    constexpr double FIXED_DT = 1.0 / 60.0;

    struct Timer
    {
        LARGE_INTEGER freq{};
        LARGE_INTEGER prev{};
        double accumulator{ 0.0 };
        double elapsedSec{ 0.0 }; // 최근 프레임 경과(렌더용)
        int    fpsCounter{ 0 };
        double fpsTimeAcc{ 0.0 };
        int    fps{ 0 };

        void Init ( )
        {
            QueryPerformanceFrequency ( &freq );
            QueryPerformanceCounter ( &prev );
        }

        // 프레임 경과 시간(초)을 누적하고 반환
        double Tick ( )
        {
            LARGE_INTEGER now{};
            QueryPerformanceCounter ( &now );
            const double dt = double ( now.QuadPart - prev.QuadPart ) / double ( freq.QuadPart );
            prev = now;
            elapsedSec = dt;
            accumulator += dt;

            // FPS 계산(1초마다)
            fpsTimeAcc += dt;
            ++fpsCounter;
            if ( fpsTimeAcc >= 1.0 )
            {
                fps = fpsCounter;
                fpsCounter = 0;
                fpsTimeAcc -= 1.0;
            }
            return dt;
        }

        bool ShouldUpdate ( ) const { return accumulator >= FIXED_DT; }
        void ConsumeStep ( ) { accumulator -= FIXED_DT; }
    };

    // 아주 단순한 입력(ESC 종료)을 위해 메시지 처리 외에 비동기 체크도 사용
    inline bool IsKeyDown ( int vk )
    {
        return ( GetAsyncKeyState ( vk ) & 0x8000 ) != 0;
    }
}

// 전역(데모용): 창 크기와 배경색
static int   gClientW = 960;
static int   gClientH = 540;
static COLORREF gClearColor = RGB ( 24 , 28 , 32 );

// 윈도우 프로시저
LRESULT CALLBACK WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
    switch ( msg )
    {
    case WM_SIZE:
        gClientW = LOWORD ( lParam );
        gClientH = HIWORD ( lParam );
        return 0;

    case WM_DESTROY:
        PostQuitMessage ( 0 );
        return 0;
    }
    return DefWindowProcW ( hWnd , msg , wParam , lParam );
}

// 데모용 Update/Render
void FixedUpdate ( ) noexcept
{
    // 여기서 게임 상태 업데이트(60Hz). 지금은 비워둠.
}

void Render ( HWND hWnd , const core::Timer& t ) noexcept
{
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint ( hWnd , &ps );

    // 배경 지우기
    HBRUSH hBrush = CreateSolidBrush ( gClearColor );
    RECT rc{ 0, 0, gClientW, gClientH };
    FillRect ( hdc , &rc , hBrush );
    DeleteObject ( hBrush );

    // 텍스트(좌상단): dt, FPS
    SetBkMode ( hdc , TRANSPARENT );
    SetTextColor ( hdc , RGB ( 240 , 240 , 240 ) );

    wchar_t buf[ 256 ];
    std::swprintf ( buf , _countof ( buf ) , L"dt(fixed): %.3f  |  fps: %d" , core::FIXED_DT , t.fps );
    int len = static_cast< int >( wcslen ( buf ) );
    TextOutW ( hdc , 8 , 8 , buf , len );

    EndPaint ( hWnd , &ps );

}

int WINAPI wWinMain ( HINSTANCE hInst , HINSTANCE , PWSTR , int nCmdShow )
{
    // 윈도우 클래스 등록
    const wchar_t* kClass = L"KirbyGameWindowClass";
    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor ( nullptr , IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszClassName = kClass;
    RegisterClassW ( &wc );

    // 창 생성 (클라이언트 크기 보정)
    RECT r{ 0,0,gClientW,gClientH };
    AdjustWindowRect ( &r , WS_OVERLAPPEDWINDOW , FALSE );
    HWND hWnd = CreateWindowExW (
        0 , kClass , L"KirbyGame (Step 0)" ,
        WS_OVERLAPPEDWINDOW , CW_USEDEFAULT , CW_USEDEFAULT ,
        r.right - r.left , r.bottom - r.top ,
        nullptr , nullptr , hInst , nullptr );

    ShowWindow ( hWnd , nCmdShow );
    UpdateWindow ( hWnd );

    // 타이머 초기화
    core::Timer timer;
    timer.Init ( );

    // 메시지 루프 + 게임 루프(고정 업데이트/가변 렌더)
    MSG msg{};
    bool running = true;
    while ( running )
    {
        // 윈도우 메시지 처리(Non-blocking)
        while ( PeekMessageW ( &msg , nullptr , 0 , 0 , PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT )
            {
                running = false;
                break;
            }
            TranslateMessage ( &msg );
            DispatchMessageW ( &msg );
        }
        if ( !running ) break;

        // ESC 종료
        if ( core::IsKeyDown ( VK_ESCAPE ) )
        {
            PostQuitMessage ( 0 );
            continue;
        }

        // 프레임 타이머 틱
        timer.Tick ( );

        // 고정 업데이트를 소화 (프레임 저하 시 여러 번 호출될 수 있음)
        // 안전을 위해 최대 스텝 제한(스파이럴 오브 데스 방지)
        int steps = 0;
        const int MAX_STEPS_PER_FRAME = 5;
        while ( timer.ShouldUpdate ( ) && steps < MAX_STEPS_PER_FRAME )
        {
            FixedUpdate ( );
            timer.ConsumeStep ( );
            ++steps;
        }

        // 가변 렌더
        InvalidateRect ( hWnd , nullptr , FALSE );
        Render ( hWnd , timer );
    }

    return 0;
}
