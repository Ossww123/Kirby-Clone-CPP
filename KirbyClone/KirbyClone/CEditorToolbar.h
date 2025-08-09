#pragma once

class CEditorCore;

// 툴바 버튼 정보 구조체
struct tToolbarButton
{
    int iButtonID;
    int iX, iY, iWidth, iHeight;
    wstring strText;
    wstring strTooltip;
    COLORREF colorNormal;
    COLORREF colorHover;
    COLORREF colorPressed;
    bool bEnabled;
    bool bPressed;
    bool bHovered;

    tToolbarButton(int id, int x, int y, int w, int h, const wstring& text, const wstring& tooltip = L"")
        : iButtonID(id), iX(x), iY(y), iWidth(w), iHeight(h), strText(text), strTooltip(tooltip)
        , colorNormal(RGB(70, 70, 70)), colorHover(RGB(90, 90, 90)), colorPressed(RGB(50, 50, 50))
        , bEnabled(true), bPressed(false), bHovered(false)
    {}
};

// 툴바 버튼 ID 열거형
enum class TOOLBAR_BUTTON_ID
{
    NEW_LEVEL,
    SAVE_LEVEL,
    LOAD_LEVEL,
    SEPARATOR_1,
    MODE_MONSTER,
    MODE_ITEM,
    MODE_TILE,
    MODE_SPECIAL,
    MODE_BACKGROUND,
    SEPARATOR_2,
    GRID_TOGGLE,
    SNAP_TOGGLE,
    UI_TOGGLE,
    SEPARATOR_3,
    MAP_SIZE_LABEL,    // 맵 크기 라벨
    MAP_SIZE_SMALL,    // 작은 맵 (1920x1080)
    MAP_SIZE_MEDIUM,   // 중간 맵 (3840x2160)
    MAP_SIZE_LARGE,    // 큰 맵 (7680x4320)
    MAP_SIZE_CUSTOM,   // 사용자 정의
    SEPARATOR_4,
    QUICK_SAVE,
    QUICK_LOAD,
    END
};

class CEditorToolbar
{
private:
    CEditorCore* m_pEditorCore;

    // 툴바 설정
    int m_iToolbarHeight;
    int m_iButtonHeight;
    int m_iButtonMargin;
    int m_iSeparatorWidth;

    // 정보 표시 영역
    int m_iInfoAreaX;
    int m_iInfoAreaWidth;


    // 버튼 목록
    vector<tToolbarButton> m_vecButtons;

    // 마우스 상태
    Vec2 m_vMousePos;
    bool m_bMouseDown;

    // 툴팁 표시
    tToolbarButton* m_pHoveredButton;
    float m_fTooltipTimer;

    // 맵 크기 관련
    Vec2 m_vCurrentMapSize;
    Vec2 m_vDefaultMapSize;

public:
    void Initialize(CEditorCore* _pCore);
    void Update();
    void Render(HDC _dc);

    // 마우스 이벤트 처리
    bool HandleMouseMove(Vec2 vMousePos);
    bool HandleMouseClick(Vec2 vMousePos);
    bool HandleMouseUp(Vec2 vMousePos);

    // 툴바 영역 체크
    bool IsInToolbarArea(Vec2 vMousePos);

    // 맵 크기 관련 메서드
    void SetMapSize(Vec2 vSize);
    Vec2 GetMapSize() const { return m_vCurrentMapSize; }
    void ShowCustomMapSizeDialog();

private:
    // 버튼 관리
    void CreateButtons();
    void UpdateButtonStates();
    void UpdateMapSizeButtons();
    tToolbarButton* GetButtonAt(Vec2 vMousePos);

    // 버튼 액션 처리
    void ExecuteButtonAction(TOOLBAR_BUTTON_ID buttonID);

    // 렌더링
    void RenderButton(HDC _dc, const tToolbarButton& button);
    void RenderSeparator(HDC _dc, int x, int y);
    void RenderTooltip(HDC _dc);
    void RenderToolbarBackground(HDC _dc);
    void RenderMapSizeLabel(HDC _dc, const tToolbarButton& button);
    void RenderInfoArea(HDC _dc);

    // 유틸리티
    COLORREF GetButtonColor(const tToolbarButton& button);
    wstring GetModeButtonText(EDITOR_MODE mode);
    bool IsModeButton(TOOLBAR_BUTTON_ID buttonID);
    bool IsMapSizeButton(TOOLBAR_BUTTON_ID buttonID);

public:
    CEditorToolbar();
    ~CEditorToolbar();

    // 접근자
    int GetToolbarHeight() const { return m_iToolbarHeight; }
};