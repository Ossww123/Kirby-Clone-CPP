#include <windows.h>
#include "engine/GameApp.h"
#include "game/GameConfig.h"

static int gClientW = game::CLIENT_W;  // 240*4 = 960
static int gClientH = game::CLIENT_H;  // 160*4 = 640
static engine::GameApp gApp;

// 윈도우 프로시저: 윈도우로 전달되는 메시지를 처리하는 콜백 함수
LRESULT CALLBACK WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
    gApp.OnWndMessage ( hWnd , msg , wParam , lParam );

    switch ( msg )
    {
    case WM_SIZE: { // 윈도우 크기 변경 메시지
        const int w = LOWORD ( lParam );        // lParam의 하위 16비트에서 너비 추출
        const int h = HIWORD ( lParam );        // lParam의 상위 16비트에서 높이 추출
        gApp.OnResize ( w , h );
        return 0;
    }
    case WM_ERASEBKGND: // 배경 지우기 요청 메시지
        return 1;                               // OS 배경 지우기 방지 (1 반환 시 처리됨으로 간주)
    case WM_PAINT: { // 윈도우 다시 그리기 메시지
        PAINTSTRUCT ps;
        BeginPaint ( hWnd , &ps );              // 그리기 시작, 무효 영역 검증
        EndPaint ( hWnd , &ps );                // 그리기 종료
        return 0;
    }
    case WM_DESTROY: // 윈도우 파괴 메시지
        PostQuitMessage ( 0 );                  // 메시지 큐에 WM_QUIT 메시지 게시 (종료 코드 0)
        return 0;
    }
    return DefWindowProcW ( hWnd , msg , wParam , lParam ); // 처리되지 않은 메시지는 기본 처리
}


int WINAPI wWinMain ( HINSTANCE hInst , HINSTANCE , PWSTR , int nCmdShow )
{
    const wchar_t* kClass = L"KirbyGameWindowClass";

    // 윈도우 클래스 구조체 정의
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;                                    // 클래스 스타일: 각 윈도우마다 고유 DC(Device Context) 할당
    wc.lpfnWndProc = WndProc;                               // 윈도우 프로시저 함수 포인터 지정
    wc.hInstance = hInst;                                   // 애플리케이션 인스턴스 핸들
    wc.hCursor = LoadCursor ( nullptr , IDC_ARROW );        // 기본 화살표 커서 로드
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );     // 배경 브러시 (시스템 윈도우 색상)
    wc.lpszClassName = kClass;                              // 윈도우 클래스 이름
    RegisterClassW ( &wc );                                 // 윈도우 클래스 등록

    // 클라이언트 영역 크기를 기준으로 윈도우 전체 크기 계산
    RECT r{ 0,0,gClientW,gClientH };
    AdjustWindowRect ( &r , WS_OVERLAPPEDWINDOW , FALSE ); // 윈도우 스타일에 맞게 RECT 조정 (타이틀바, 테두리 포함)

    // 윈도우 생성
    HWND hWnd = CreateWindowExW (
        0 ,                          // 확장 윈도우 스타일
        kClass ,                     // 등록된 윈도우 클래스 이름
        L"KirbyGame" ,               // 윈도우 제목
        WS_OVERLAPPEDWINDOW ,        // 윈도우 스타일 (타이틀바, 최소/최대화 버튼, 크기 조정 가능)
        CW_USEDEFAULT ,              // X 위치 (기본값 사용)
        CW_USEDEFAULT ,              // Y 위치 (기본값 사용)
        r.right - r.left ,           // 윈도우 너비
        r.bottom - r.top ,           // 윈도우 높이
        nullptr ,                    // 부모 윈도우 핸들 (없음)
        nullptr ,                    // 메뉴 핸들 (없음)
        hInst ,                      // 애플리케이션 인스턴스
        nullptr );                   // 추가 매개변수 (없음)

    ShowWindow ( hWnd , nCmdShow );  // 윈도우 표시 (nCmdShow에 따라 최대화/최소화 등)
    UpdateWindow ( hWnd );           // WM_PAINT 메시지를 즉시 전송하여 윈도우 업데이트

    gApp.Init ( hWnd );

    // 메시지 루프
    MSG msg{};
    bool running = true;
    while ( running )
    {
        // 메시지 큐에서 메시지 확인 및 처리 (대기하지 않음)
        while ( PeekMessageW ( &msg , nullptr , 0 , 0 , PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) { running = false; break; } // 종료 메시지 확인
            TranslateMessage ( &msg ); // 가상 키 메시지를 문자 메시지로 변환
            DispatchMessageW ( &msg ); // 메시지를 해당 윈도우 프로시저로 전달
        }
        if ( !running ) break;

        // 한 프레임 처리
        if ( !gApp.DoOneFrame ( ) ) break;
    }
    return 0;
}
