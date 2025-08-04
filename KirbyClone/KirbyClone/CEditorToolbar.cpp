#include "pch.h"
#include "CEditorToolbar.h"
#include "CEditorCore.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"

#include "CGrid.h"
#include "CCore.h"

#include "CTimeMgr.h"

CEditorToolbar::CEditorToolbar()
    : m_pEditorCore(nullptr)
    , m_iToolbarHeight(50)
    , m_iButtonHeight(30)
    , m_iButtonMargin(5)
    , m_iSeparatorWidth(10)
    , m_vMousePos(0.f, 0.f)
    , m_bMouseDown(false)
    , m_pHoveredButton(nullptr)
    , m_fTooltipTimer(0.f)
{
}

CEditorToolbar::~CEditorToolbar()
{
}

void CEditorToolbar::Initialize(CEditorCore* _pCore)
{
    m_pEditorCore = _pCore;
    CreateButtons();
}

void CEditorToolbar::Update()
{
    UpdateButtonStates();

    // 툴팁 타이머 업데이트
    if (m_pHoveredButton)
    {
        m_fTooltipTimer += CTimeMgr::GetInst()->GetfDT();
    }
    else
    {
        m_fTooltipTimer = 0.f;
    }
}

void CEditorToolbar::Render(HDC _dc)
{
    // 툴바 배경 렌더링
    RenderToolbarBackground(_dc);

    // 버튼들 렌더링
    for (const auto& button : m_vecButtons)
    {
        if (button.iButtonID != (int)TOOLBAR_BUTTON_ID::SEPARATOR_1 &&
            button.iButtonID != (int)TOOLBAR_BUTTON_ID::SEPARATOR_2 &&
            button.iButtonID != (int)TOOLBAR_BUTTON_ID::SEPARATOR_3)
        {
            RenderButton(_dc, button);
        }
        else
        {
            RenderSeparator(_dc, button.iX, button.iY);
        }
    }

    // 툴팁 렌더링 (마우스가 버튼 위에 1초 이상 있을 때)
    if (m_pHoveredButton && m_fTooltipTimer > 1.0f)
    {
        RenderTooltip(_dc);
    }
}

bool CEditorToolbar::HandleMouseMove(Vec2 vMousePos)
{
    m_vMousePos = vMousePos;

    // 이전 호버 상태 초기화
    if (m_pHoveredButton)
    {
        m_pHoveredButton->bHovered = false;
        m_pHoveredButton = nullptr;
    }

    // 현재 마우스 위치의 버튼 확인
    tToolbarButton* pButton = GetButtonAt(vMousePos);
    if (pButton && pButton->bEnabled)
    {
        pButton->bHovered = true;
        m_pHoveredButton = pButton;
        return true; // 툴바에서 마우스 이벤트 처리함
    }

    return IsInToolbarArea(vMousePos); // 툴바 영역에 있으면 이벤트 소비
}

bool CEditorToolbar::HandleMouseClick(Vec2 vMousePos)
{
    if (!IsInToolbarArea(vMousePos))
        return false;

    m_bMouseDown = true;

    tToolbarButton* pButton = GetButtonAt(vMousePos);
    if (pButton && pButton->bEnabled)
    {
        pButton->bPressed = true;
        return true;
    }

    return true; // 툴바 영역 클릭은 항상 소비
}

bool CEditorToolbar::HandleMouseUp(Vec2 vMousePos)
{
    if (!m_bMouseDown)
        return false;

    m_bMouseDown = false;

    // 모든 버튼의 pressed 상태 해제
    for (auto& button : m_vecButtons)
    {
        button.bPressed = false;
    }

    if (!IsInToolbarArea(vMousePos))
        return false;

    // 버튼 액션 실행
    tToolbarButton* pButton = GetButtonAt(vMousePos);
    if (pButton && pButton->bEnabled)
    {
        ExecuteButtonAction((TOOLBAR_BUTTON_ID)pButton->iButtonID);
        return true;
    }

    return true;
}

bool CEditorToolbar::IsInToolbarArea(Vec2 vMousePos)
{
    return vMousePos.y >= 0 && vMousePos.y <= m_iToolbarHeight;
}

void CEditorToolbar::CreateButtons()
{
    m_vecButtons.clear();

    int currentX = m_iButtonMargin;
    int buttonY = (m_iToolbarHeight - m_iButtonHeight) / 2;

    // 파일 관련 버튼
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::NEW_LEVEL, currentX, buttonY, 60, m_iButtonHeight, L"New", L"새 레벨 생성");
    currentX += 60 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::SAVE_LEVEL, currentX, buttonY, 60, m_iButtonHeight, L"Save", L"레벨 저장 (Ctrl+S)");
    currentX += 60 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::LOAD_LEVEL, currentX, buttonY, 60, m_iButtonHeight, L"Load", L"레벨 불러오기 (Ctrl+O)");
    currentX += 60 + m_iButtonMargin;

    // 구분선 1
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::SEPARATOR_1, currentX, buttonY, m_iSeparatorWidth, m_iButtonHeight, L"", L"");
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 모드 버튼들
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::MODE_MONSTER, currentX, buttonY, 70, m_iButtonHeight, L"Monster", L"몬스터 배치 모드 (M)");
    currentX += 70 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::MODE_ITEM, currentX, buttonY, 50, m_iButtonHeight, L"Item", L"아이템 배치 모드 (I)");
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::MODE_TILE, currentX, buttonY, 50, m_iButtonHeight, L"Tile", L"타일 배치 모드 (T)");
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::MODE_BACKGROUND, currentX, buttonY, 80, m_iButtonHeight, L"Background", L"배경 모드 (B)");
    currentX += 80 + m_iButtonMargin;

    // 구분선 2
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::SEPARATOR_2, currentX, buttonY, m_iSeparatorWidth, m_iButtonHeight, L"", L"");
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 그리드/스냅 버튼들
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::GRID_TOGGLE, currentX, buttonY, 50, m_iButtonHeight, L"Grid", L"그리드 표시 토글 (G)");
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::SNAP_TOGGLE, currentX, buttonY, 50, m_iButtonHeight, L"Snap", L"그리드 스냅 토글 (F)");
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::UI_TOGGLE, currentX, buttonY, 40, m_iButtonHeight, L"UI", L"UI 표시 토글 (H)");
    currentX += 40 + m_iButtonMargin;

    // 구분선 3
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::SEPARATOR_3, currentX, buttonY, m_iSeparatorWidth, m_iButtonHeight, L"", L"");
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 빠른 저장/로드
    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::QUICK_SAVE, currentX, buttonY, 40, m_iButtonHeight, L"Q.S", L"빠른 저장 (F5)");
    currentX += 40 + m_iButtonMargin;

    m_vecButtons.emplace_back((int)TOOLBAR_BUTTON_ID::QUICK_LOAD, currentX, buttonY, 40, m_iButtonHeight, L"Q.L", L"빠른 로드 (F9)");
}

void CEditorToolbar::UpdateButtonStates()
{
    // 현재 모드에 따라 모드 버튼들의 색상 변경
    EDITOR_MODE currentMode = m_pEditorCore->GetCurrentMode();

    for (auto& button : m_vecButtons)
    {
        // 모드 버튼들의 활성 상태 표시
        if (IsModeButton((TOOLBAR_BUTTON_ID)button.iButtonID))
        {
            bool isActive = false;

            switch ((TOOLBAR_BUTTON_ID)button.iButtonID)
            {
            case TOOLBAR_BUTTON_ID::MODE_MONSTER:
                isActive = (currentMode == EDITOR_MODE::PLACE_MONSTER);
                break;
            case TOOLBAR_BUTTON_ID::MODE_ITEM:
                isActive = (currentMode == EDITOR_MODE::PLACE_ITEM);
                break;
            case TOOLBAR_BUTTON_ID::MODE_TILE:
                isActive = (currentMode == EDITOR_MODE::PLACE_TILE);
                break;
            case TOOLBAR_BUTTON_ID::MODE_BACKGROUND:
                isActive = (currentMode == EDITOR_MODE::BACKGROUND);
                break;
            }

            if (isActive)
            {
                button.colorNormal = RGB(100, 150, 100);
                button.colorHover = RGB(120, 170, 120);
            }
            else
            {
                button.colorNormal = RGB(70, 70, 70);
                button.colorHover = RGB(90, 90, 90);
            }
        }

        // 그리드/스냅 버튼 상태 업데이트
        if (button.iButtonID == (int)TOOLBAR_BUTTON_ID::GRID_TOGGLE)
        {
            if (CGrid::GetInst()->IsShowGrid())
            {
                button.colorNormal = RGB(100, 150, 100);
                button.colorHover = RGB(120, 170, 120);
            }
            else
            {
                button.colorNormal = RGB(70, 70, 70);
                button.colorHover = RGB(90, 90, 90);
            }
        }

        if (button.iButtonID == (int)TOOLBAR_BUTTON_ID::SNAP_TOGGLE)
        {
            if (CGrid::GetInst()->IsSnapToGrid())
            {
                button.colorNormal = RGB(100, 150, 100);
                button.colorHover = RGB(120, 170, 120);
            }
            else
            {
                button.colorNormal = RGB(70, 70, 70);
                button.colorHover = RGB(90, 90, 90);
            }
        }

        if (button.iButtonID == (int)TOOLBAR_BUTTON_ID::UI_TOGGLE)
        {
            if (m_pEditorCore->IsShowUI())
            {
                button.colorNormal = RGB(100, 150, 100);
                button.colorHover = RGB(120, 170, 120);
            }
            else
            {
                button.colorNormal = RGB(70, 70, 70);
                button.colorHover = RGB(90, 90, 90);
            }
        }
    }
}

tToolbarButton* CEditorToolbar::GetButtonAt(Vec2 vMousePos)
{
    for (auto& button : m_vecButtons)
    {
        if (vMousePos.x >= button.iX && vMousePos.x <= button.iX + button.iWidth &&
            vMousePos.y >= button.iY && vMousePos.y <= button.iY + button.iHeight)
        {
            return &button;
        }
    }
    return nullptr;
}

void CEditorToolbar::ExecuteButtonAction(TOOLBAR_BUTTON_ID buttonID)
{
    switch (buttonID)
    {
    case TOOLBAR_BUTTON_ID::NEW_LEVEL:
        m_pEditorCore->GetObjectManager()->ClearAllObjects();
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"New level created");
        break;

    case TOOLBAR_BUTTON_ID::SAVE_LEVEL:
        m_pEditorCore->GetFileManager()->SaveAsDialog();
        break;

    case TOOLBAR_BUTTON_ID::LOAD_LEVEL:
        m_pEditorCore->GetFileManager()->OpenDialog();
        break;

    case TOOLBAR_BUTTON_ID::MODE_MONSTER:
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_MONSTER);
        break;

    case TOOLBAR_BUTTON_ID::MODE_ITEM:
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_ITEM);
        break;

    case TOOLBAR_BUTTON_ID::MODE_TILE:
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_TILE);
        break;

    case TOOLBAR_BUTTON_ID::MODE_BACKGROUND:
        m_pEditorCore->ChangeMode(EDITOR_MODE::BACKGROUND);
        break;

    case TOOLBAR_BUTTON_ID::GRID_TOGGLE:
    {
        bool bVisible = CGrid::GetInst()->IsShowGrid();
        CGrid::GetInst()->SetShowGrid(!bVisible);
    }
    break;

    case TOOLBAR_BUTTON_ID::SNAP_TOGGLE:
    {
        bool bSnap = CGrid::GetInst()->IsSnapToGrid();
        CGrid::GetInst()->SetSnapToGrid(!bSnap);
    }
    break;

    case TOOLBAR_BUTTON_ID::UI_TOGGLE:
    {
        bool bShowUI = m_pEditorCore->IsShowUI();
        m_pEditorCore->SetShowUI(!bShowUI);
    }
    break;

    case TOOLBAR_BUTTON_ID::QUICK_SAVE:
        m_pEditorCore->GetFileManager()->QuickSave();
        break;

    case TOOLBAR_BUTTON_ID::QUICK_LOAD:
        m_pEditorCore->GetFileManager()->QuickLoad();
        break;
    }
}

void CEditorToolbar::RenderButton(HDC _dc, const tToolbarButton& button)
{
    // 버튼 배경 색상 결정
    COLORREF bgColor = GetButtonColor(button);

    // 버튼 배경 그리기
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 버튼 테두리
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    Rectangle(_dc, button.iX, button.iY, button.iX + button.iWidth, button.iY + button.iHeight);

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);

    // 버튼 텍스트 그리기
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(255, 255, 255));

    HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    // 텍스트 중앙 정렬
    SIZE textSize;
    GetTextExtentPoint32(_dc, button.strText.c_str(), (int)button.strText.length(), &textSize);

    int textX = button.iX + (button.iWidth - textSize.cx) / 2;
    int textY = button.iY + (button.iHeight - textSize.cy) / 2;

    TextOut(_dc, textX, textY, button.strText.c_str(), (int)button.strText.length());

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
}

void CEditorToolbar::RenderSeparator(HDC _dc, int x, int y)
{
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    int separatorX = x + m_iSeparatorWidth / 2;
    MoveToEx(_dc, separatorX, y + 5, nullptr);
    LineTo(_dc, separatorX, y + m_iButtonHeight - 5);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CEditorToolbar::RenderTooltip(HDC _dc)
{
    if (!m_pHoveredButton || m_pHoveredButton->strTooltip.empty())
        return;

    // 툴팁 텍스트 크기 계산
    SIZE textSize;
    GetTextExtentPoint32(_dc, m_pHoveredButton->strTooltip.c_str(),
        (int)m_pHoveredButton->strTooltip.length(), &textSize);

    // 툴팁 박스 위치 계산
    int tooltipX = (int)m_vMousePos.x + 10;
    int tooltipY = m_iToolbarHeight + 5;
    int tooltipWidth = textSize.cx + 10;
    int tooltipHeight = textSize.cy + 6;

    // 화면 경계 체크
    RECT clientRect;
    GetClientRect(CCore::GetInst()->GetMainHwnd(), &clientRect);
    if (tooltipX + tooltipWidth > clientRect.right)
        tooltipX = (int)m_vMousePos.x - tooltipWidth - 10;

    // 툴팁 배경
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 200));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    Rectangle(_dc, tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight);

    // 툴팁 텍스트
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(0, 0, 0));

    HFONT hFont = CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    TextOut(_dc, tooltipX + 5, tooltipY + 3, m_pHoveredButton->strTooltip.c_str(),
        (int)m_pHoveredButton->strTooltip.length());

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldFont);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    DeleteObject(hFont);
}

void CEditorToolbar::RenderToolbarBackground(HDC _dc)
{
    // 툴바 배경
    HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    RECT toolbarRect = { 0, 0, 1920, m_iToolbarHeight }; // 충분히 넓게
    FillRect(_dc, &toolbarRect, hBrush);

    // 하단 경계선
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    MoveToEx(_dc, 0, m_iToolbarHeight - 1, nullptr);
    LineTo(_dc, 1920, m_iToolbarHeight - 1);

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

COLORREF CEditorToolbar::GetButtonColor(const tToolbarButton& button)
{
    if (!button.bEnabled)
        return RGB(40, 40, 40);
    else if (button.bPressed)
        return button.colorPressed;
    else if (button.bHovered)
        return button.colorHover;
    else
        return button.colorNormal;
}

wstring CEditorToolbar::GetModeButtonText(EDITOR_MODE mode)
{
    switch (mode)
    {
    case EDITOR_MODE::PLACE_MONSTER: return L"Monster";
    case EDITOR_MODE::PLACE_ITEM: return L"Item";
    case EDITOR_MODE::PLACE_TILE: return L"Tile";
    case EDITOR_MODE::BACKGROUND: return L"Background";
    default: return L"Unknown";
    }
}

bool CEditorToolbar::IsModeButton(TOOLBAR_BUTTON_ID buttonID)
{
    return buttonID == TOOLBAR_BUTTON_ID::MODE_MONSTER ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_ITEM ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_TILE ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_BACKGROUND;
}