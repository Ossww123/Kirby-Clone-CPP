#include "pch.h"
#include "CEditorToolbar.h"
#include "CEditorCore.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"
#include "CEditorCameraController.h"

#include "CGrid.h"
#include "CCore.h"
#include "CStageMgr.h"
#include "CStageImage.h"

#include "CTimeMgr.h"

CEditorToolbar::CEditorToolbar()
    : m_pEditorCore(nullptr)
    , m_iToolbarHeight(50)
    , m_iButtonHeight(30)
    , m_iButtonMargin(5)
    , m_iSeparatorWidth(10)
    , m_iInfoAreaX(0)
    , m_iInfoAreaWidth(0)
    , m_vMousePos(0.f, 0.f)
    , m_bMouseDown(false)
    , m_pHoveredButton(nullptr)
    , m_fTooltipTimer(0.f)
    , m_vCurrentMapSize(3840.f, 2160.f)
    , m_vDefaultMapSize(3840.f, 2160.f)
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
            button.iButtonID != (int)TOOLBAR_BUTTON_ID::SEPARATOR_3 &&
            button.iButtonID != (int)TOOLBAR_BUTTON_ID::SEPARATOR_4)
        {
            if (button.iButtonID == (int)TOOLBAR_BUTTON_ID::MAP_SIZE_LABEL)
            {
                RenderMapSizeLabel(_dc, button);
            }
            else
            {
                RenderButton(_dc, button);
            }
        }
        else
        {
            RenderSeparator(_dc, button.iX, button.iY);
        }
    }

    // 맵 크기 및 카메라 좌표 정보 표시 추가
    RenderInfoArea(_dc);

    // 툴팁 렌더링 (마우스가 버튼 위에 1초 이상 있을 때)
    if (m_pHoveredButton && m_fTooltipTimer > 1.0f)
    {
        RenderTooltip(_dc);
    }
}

bool CEditorToolbar::HandleMouseMove(Vec2 vMousePos)
{
    m_vMousePos = vMousePos;

    // 기존 호버 상태 초기화
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
        return true; // 툴바에서 마우스 이벤트 처리됨
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

    return true;
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

void CEditorToolbar::SetMapSize(Vec2 vSize)
{
    m_vCurrentMapSize = vSize;

    // 에디터 자체의 맵 크기 설정
    if (m_pEditorCore)
    {
        m_pEditorCore->SetMapSize(vSize);
    }

    if (m_pEditorCore && m_pEditorCore->GetCameraController())
    {
        // 월드 카메라 범위: (0,0) ~ (width, height) - 월드 좌표를 그대로
        Vec2 vMin = Vec2(0.f, 0.f);
        Vec2 vMax = Vec2(vSize.x, vSize.y);

        m_pEditorCore->GetCameraController()->SetCameraBounds(vMin, vMax);

        // 카메라 UI의 (0,0) 위치로 이동 = 월드 (0, height)
        Vec2 vUIZeroPos = Vec2(0.f, vSize.y);
        m_pEditorCore->GetCameraController()->SetCameraPosition(vUIZeroPos);
        
        // 스테이지 이미지 위치를 새로운 맵 크기에 맞게 업데이트
        CStageImage* pCurrentStage = CStageMgr::GetInst()->GetCurrentStageImage();
        if (pCurrentStage)
        {
            pCurrentStage->SetImageToBottomLeft(vSize);
        }
    }

    UpdateMapSizeButtons();
}

void CEditorToolbar::UpdateMapSizeButtons()
{
    for (auto& button : m_vecButtons)
    {
        if (IsMapSizeButton((TOOLBAR_BUTTON_ID)button.iButtonID))
        {
            bool isActive = false;

            switch ((TOOLBAR_BUTTON_ID)button.iButtonID)
            {
            case TOOLBAR_BUTTON_ID::MAP_SIZE_SMALL:
                isActive = (m_vCurrentMapSize.x == 1920.f && m_vCurrentMapSize.y == 1080.f);
                break;
            case TOOLBAR_BUTTON_ID::MAP_SIZE_MEDIUM:
                isActive = (m_vCurrentMapSize.x == 3840.f && m_vCurrentMapSize.y == 2160.f);
                break;
            case TOOLBAR_BUTTON_ID::MAP_SIZE_LARGE:
                isActive = (m_vCurrentMapSize.x == 7680.f && m_vCurrentMapSize.y == 4320.f);
                break;
            case TOOLBAR_BUTTON_ID::MAP_SIZE_CUSTOM:
                isActive = !(m_vCurrentMapSize.x == 1920.f && m_vCurrentMapSize.y == 1080.f) &&
                    !(m_vCurrentMapSize.x == 3840.f && m_vCurrentMapSize.y == 2160.f) &&
                    !(m_vCurrentMapSize.x == 7680.f && m_vCurrentMapSize.y == 4320.f);
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
    }
}

void CEditorToolbar::ShowCustomMapSizeDialog()
{
    wchar_t szMessage[512];
    swprintf_s(szMessage,
        L"Current Map Size: %.0f x %.0f\n\n"
        L"Copy 'width,height' format to clipboard and click OK\n"
        L"Example: 1920,1080\n"
        L"Minimum Size: 960x640 (screen resolution)\n\n"
        L"Click OK after copying to clipboard.",
        m_vCurrentMapSize.x, m_vCurrentMapSize.y);

    if (MessageBox(CCore::GetInst()->GetMainHwnd(), szMessage, L"Custom Map Size", MB_OKCANCEL) != IDOK)
        return;

    // 클립보드에서 읽기
    wchar_t szInput[128] = L"";
    if (OpenClipboard(CCore::GetInst()->GetMainHwnd()))
    {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData)
        {
            wchar_t* pText = (wchar_t*)GlobalLock(hData);
            if (pText)
            {
                wcsncpy_s(szInput, 128, pText, _TRUNCATE);
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }

    // 좌표값 분리
    wchar_t* pComma = wcschr(szInput, L',');
    if (!pComma)
    {
        MessageBox(CCore::GetInst()->GetMainHwnd(), L"Invalid format: Enter 'width,height' format", L"Error", MB_OK);
        return;
    }

    *pComma = L'\0';
    float width = (float)_wtof(szInput);
    float height = (float)_wtof(pComma + 1);

    // 최소값 검사: 960x640 (화면 해상도)
    if (width < 960 || width > 20000 || height < 640 || height > 20000)
    {
        MessageBox(CCore::GetInst()->GetMainHwnd(),
            L"Size range error:\nWidth: 960-20000\nHeight: 640-20000\n(Minimum: 960x640)",
            L"Error", MB_OK);
        return;
    }

    SetMapSize(Vec2(width, height));
}


void CEditorToolbar::CreateButtons ( )
{
    m_vecButtons.clear ( );

    int currentX = m_iButtonMargin;
    int buttonY = ( m_iToolbarHeight - m_iButtonHeight ) / 2;

    // 파일 관련 버튼들 (텍스트 변경)
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::NEW_LEVEL , currentX , buttonY , 60 , m_iButtonHeight , L"New" , L"새 레벨 생성" );
    currentX += 60 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SAVE_LEVEL , currentX , buttonY , 60 , m_iButtonHeight , L"Save" , L"레벨 저장 (Ctrl+S)" );
    currentX += 60 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::LOAD_LEVEL , currentX , buttonY , 60 , m_iButtonHeight , L"Load" , L"레벨 불러오기 (Ctrl+O)" );
    currentX += 60 + m_iButtonMargin;

    // 분리선 1
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SEPARATOR_1 , currentX , buttonY , m_iSeparatorWidth , m_iButtonHeight , L"" , L"" );
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 모드 버튼들 (텍스트 변경)
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MODE_MONSTER , currentX , buttonY , 70 , m_iButtonHeight , L"Monster" , L"몬스터 배치 모드 (M)" );
    currentX += 70 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MODE_ITEM , currentX , buttonY , 50 , m_iButtonHeight , L"Item" , L"아이템 배치 모드 (I)" );
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MODE_TILE , currentX , buttonY , 50 , m_iButtonHeight , L"Tile" , L"타일 배치 모드 (T)" );
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MODE_SPECIAL , currentX , buttonY , 65 , m_iButtonHeight , L"Special" , L"특수 오브젝트 배치 모드 (S)" );
    currentX += 65 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MODE_BACKGROUND , currentX , buttonY , 80 , m_iButtonHeight , L"Background" , L"배경 모드 (B)" );
    currentX += 80 + m_iButtonMargin;

    // 분리선 2
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SEPARATOR_2 , currentX , buttonY , m_iSeparatorWidth , m_iButtonHeight , L"" , L"" );
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 그리드/스냅 버튼들
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::GRID_TOGGLE , currentX , buttonY , 50 , m_iButtonHeight , L"Grid" , L"그리드 표시 토글 (G)" );
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SNAP_TOGGLE , currentX , buttonY , 50 , m_iButtonHeight , L"Snap" , L"그리드 스냅 토글 (F)" );
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::UI_TOGGLE , currentX , buttonY , 40 , m_iButtonHeight , L"UI" , L"UI 표시 토글 (H)" );
    currentX += 40 + m_iButtonMargin;

    // 분리선 3
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SEPARATOR_3 , currentX , buttonY , m_iSeparatorWidth , m_iButtonHeight , L"" , L"" );
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 맵 크기 레이블 및 버튼들
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MAP_SIZE_LABEL , currentX , buttonY , 60 , m_iButtonHeight , L"Map Size:" , L"" );
    currentX += 60 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MAP_SIZE_SMALL , currentX , buttonY , 45 , m_iButtonHeight , L"Small" , L"Small Map (1920x1080)" );
    currentX += 45 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MAP_SIZE_MEDIUM , currentX , buttonY , 50 , m_iButtonHeight , L"Medium" , L"Medium Map (3840x2160)" );
    currentX += 50 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MAP_SIZE_LARGE , currentX , buttonY , 45 , m_iButtonHeight , L"Large" , L"Large Map (7680x4320)" );
    currentX += 45 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::MAP_SIZE_CUSTOM , currentX , buttonY , 55 , m_iButtonHeight , L"Custom" , L"Custom Map Size" );
    currentX += 55 + m_iButtonMargin;

    // 분리선 4
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::SEPARATOR_4 , currentX , buttonY , m_iSeparatorWidth , m_iButtonHeight , L"" , L"" );
    currentX += m_iSeparatorWidth + m_iButtonMargin;

    // 빠른 저장/로드
    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::QUICK_SAVE , currentX , buttonY , 40 , m_iButtonHeight , L"Q.S" , L"빠른 저장 (F5)" );
    currentX += 40 + m_iButtonMargin;

    m_vecButtons.emplace_back ( ( int ) TOOLBAR_BUTTON_ID::QUICK_LOAD , currentX , buttonY , 40 , m_iButtonHeight , L"Q.L" , L"빠른 로드 (F9)" );
    currentX += 40 + m_iButtonMargin;

    // 정보 표시 영역 설정 (버튼들 오른쪽 끝)
    m_iInfoAreaX = currentX + 20;  // 버튼 끝에서 20픽셀 간격
    m_iInfoAreaWidth = 300;        // 정보 표시 영역 너비
}

void CEditorToolbar::UpdateButtonStates()
{
    // 현재 모드에 따른 모드 버튼들의 상태 설정
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
            case TOOLBAR_BUTTON_ID::MODE_SPECIAL:
                isActive = (currentMode == EDITOR_MODE::PLACE_SPECIAL);
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

    // 맵 크기 버튼 상태 업데이트
    UpdateMapSizeButtons();
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

    case TOOLBAR_BUTTON_ID::MODE_SPECIAL:
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_SPECIAL);
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

    case TOOLBAR_BUTTON_ID::MAP_SIZE_SMALL:
        SetMapSize(Vec2(1920.f, 1080.f));
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Map size set to Small (1920x1080)");
        break;

    case TOOLBAR_BUTTON_ID::MAP_SIZE_MEDIUM:
        SetMapSize(Vec2(3840.f, 2160.f));
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Map size set to Medium (3840x2160)");
        break;

    case TOOLBAR_BUTTON_ID::MAP_SIZE_LARGE:
        SetMapSize(Vec2(7680.f, 4320.f));
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Map size set to Large (7680x4320)");
        break;

    case TOOLBAR_BUTTON_ID::MAP_SIZE_CUSTOM:
        ShowCustomMapSizeDialog();
        break;
    }
}

void CEditorToolbar::RenderButton(HDC _dc, const tToolbarButton& button)
{
    // 버튼 배경 색상 설정
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

    // 툴팁 텍스트 크기 측정
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

    RECT toolbarRect = { 0, 0, 1920, m_iToolbarHeight };
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

void CEditorToolbar::RenderMapSizeLabel(HDC _dc, const tToolbarButton& button)
{
    // 일반 버튼과 다른게 렌더링 (배경 없음)
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(200, 200, 200));

    HFONT hFont = CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
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

void CEditorToolbar::RenderInfoArea(HDC _dc)
{
    // 카메라 좌표 가져오기 (월드 좌표)
    Vec2 vCameraPos(0.f, 0.f);
    if (m_pEditorCore && m_pEditorCore->GetCameraController())
    {
        vCameraPos = m_pEditorCore->GetCameraController()->GetCameraPosition();
    }

    // UI용 좌표 변환: 월드 Y를 UI Y로 변환
    float uiY = m_vCurrentMapSize.y - vCameraPos.y;

    // 정보 텍스트 생성 (UI 좌표로 표시)
    wchar_t szMapInfo[128];
    wchar_t szCameraInfo[128];

    swprintf_s(szMapInfo, L"Map: %.0fx%.0f", m_vCurrentMapSize.x, m_vCurrentMapSize.y);
    swprintf_s(szCameraInfo, L"Cam: (%.0f, %.0f)", vCameraPos.x, uiY);

    // 텍스트 렌더링 설정
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(200, 200, 200));

    HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    int yPos = (m_iToolbarHeight - 24) / 2;

    TextOut(_dc, m_iInfoAreaX, yPos, szMapInfo, (int)wcslen(szMapInfo));
    TextOut(_dc, m_iInfoAreaX, yPos + 12, szCameraInfo, (int)wcslen(szCameraInfo));

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
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
    case EDITOR_MODE::PLACE_SPECIAL: return L"Special";
    case EDITOR_MODE::BACKGROUND: return L"Background";
    default: return L"Unknown";
    }
}

bool CEditorToolbar::IsModeButton(TOOLBAR_BUTTON_ID buttonID)
{
    return buttonID == TOOLBAR_BUTTON_ID::MODE_MONSTER ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_ITEM ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_TILE ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_SPECIAL ||
        buttonID == TOOLBAR_BUTTON_ID::MODE_BACKGROUND;
}

bool CEditorToolbar::IsMapSizeButton(TOOLBAR_BUTTON_ID buttonID)
{
    return buttonID == TOOLBAR_BUTTON_ID::MAP_SIZE_SMALL ||
        buttonID == TOOLBAR_BUTTON_ID::MAP_SIZE_MEDIUM ||
        buttonID == TOOLBAR_BUTTON_ID::MAP_SIZE_LARGE ||
        buttonID == TOOLBAR_BUTTON_ID::MAP_SIZE_CUSTOM;
}
