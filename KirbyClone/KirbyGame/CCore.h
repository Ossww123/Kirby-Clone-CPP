#pragma once

class CCore
{
    SINGLE(CCore);

public:
    // === 게임 상수들 ===
    static constexpr int GAME_WIDTH = 960;      // 게임보이 해상도 4배 (240*4)
    static constexpr int GAME_HEIGHT = 640;     // 게임보이 해상도 4배 (160*4)
    static constexpr float PIXEL_SCALE = 4.0f;  // 픽셀아트 4배 확대

    // === 윈도우 위치 상수 ===
    static constexpr int WINDOW_POS_X = 100;    // 윈도우 기본 X 위치
    static constexpr int WINDOW_POS_Y = 100;    // 윈도우 기본 Y 위치

public:
    // === 메인 진입점 함수들 ===
    int init(HWND _hWnd, POINT _ptResolution);
    void progress();

private:
    void update();          // progress()에서 호출 - 전체게임 로직 체크
    void render();          // progress()에서 호출 - 화면 렌더링

public:
    // === 해상도 설정 ===
    void SetGameResolution();                           // 게임 해상도로 설정 (960x640)

private:
    // === 해상도 변경 내부 함수 ===
    void ChangeResolution(int _iWidth, int _iHeight);   // 해상도 변경
    void RecreateBackBuffer();                          // 백버퍼 재생성 함수
    void UpdateWindowSize();                            // 윈도우 크기 변경 함수

public:
    // === Getter 함수들 ===
    HDC GetMainDC() const { return m_hDC; }
    HWND GetMainHwnd() const { return m_hWnd; }
    Vec2 GetResolution() const { return Vec2((float)m_ptResolution.x, (float)m_ptResolution.y); }

    // === 정적 유틸리티 함수들 ===
    static float GetPixelScale() { return PIXEL_SCALE; }
    static Vec2 GetGameBoyResolution() { return Vec2(160.f, 144.f); }
    

private:
    // === 윈도우 변수들 ===
    HWND    m_hWnd;         // 메인 윈도우 핸들
    POINT   m_ptResolution; // 메인 윈도우 해상도
    HDC     m_hDC;          // 메인 윈도우에 Draw 할 DC

    // 더블 버퍼링용
    HBITMAP m_hBit;         // 백버퍼용 비트맵 (화면 크기와 동일)
    HDC     m_memDC;        // 백버퍼용 메모리 DC (더블버퍼링 핵심)
    
};