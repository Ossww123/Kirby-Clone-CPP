#pragma once

class CCore
{
    SINGLE(CCore);

public:
    // === 정적 상수들 ===
    static constexpr int GAME_WIDTH = 960;      // 게임보이 4배 (240*4)
    static constexpr int GAME_HEIGHT = 640;     // 게임보이 4배 (160*4)
    static constexpr int TOOL_WIDTH = 1920;     // 툴 해상도 (레벨 에디터)
    static constexpr int TOOL_HEIGHT = 1080;    // 툴 해상도 (레벨 에디터)
    static constexpr float PIXEL_SCALE = 4.0f;  // 스프라이트 4배 확대

public:
    // === 핵심 생명주기 함수들 ===
    int init(HWND _hWnd, POINT _ptResolution);
    void progress();

private:
    void update();          // progress()에서 호출 - 물체들의 변경점 체크
    void render();          // progress()에서 호출 - 화면 렌더링

public:
    // === 해상도 관리 ===
    void SetGameResolution();                           // 게임 해상도로 변경 (960x640)
    void SetToolResolution();                           // 툴 해상도로 변경 (1920x1080)

private:
    // === 해상도 관리 내부 구현 ===
    void ChangeResolution(int _iWidth, int _iHeight);   // 해상도 변경
    void RecreateBackBuffer();                          // 백버퍼 재생성 분리
    void UpdateWindowSize();                            // 윈도우 크기 조정 분리

public:
    // === Getter 함수들 ===
    HDC GetMainDC() const { return m_hDC; }
    HWND GetMainHwnd() const { return m_hWnd; }
    Vec2 GetResolution() const { return Vec2((float)m_ptResolution.x, (float)m_ptResolution.y); }

    // === 정적 유틸리티 함수들 ===
    static float GetPixelScale() { return PIXEL_SCALE; }
    static Vec2 GetGameBoyResolution() { return Vec2(160.f, 144.f); }

private:
    // === 멤버 변수들 ===
    HWND    m_hWnd;         // 메인 윈도우 핸들
    POINT   m_ptResolution; // 메인 윈도우 해상도
    HDC     m_hDC;          // 메인 윈도우에 Draw 할 DC

    // 더블 버퍼링용
    HBITMAP m_hBit;         // 백버퍼용 비트맵 (화면 크기와 동일)
    HDC     m_memDC;        // 백버퍼용 메모리 DC (오프스크린 렌더링)
};