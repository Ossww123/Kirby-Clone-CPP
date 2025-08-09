#include "pch.h"
#include "CEditorUI.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"
#include "CEditorToolbar.h"
#include "CEditorCameraController.h"

#include "CCore.h"
#include "CScene.h"
#include "CGrid.h"
#include "CBackgroundMgr.h"
#include "CTexture.h"
#include "CStageImage.h"
#include "CStageMgr.h"

CEditorUI::CEditorUI()
    : m_pEditorCore(nullptr)
    , m_pScene(nullptr)
    , m_iUIWidth(450)
    , m_iUIHeight(800)
    , m_iUIMargin(10)
    , m_iLineHeight(18)
{
}

CEditorUI::~CEditorUI()
{
}

void CEditorUI::Initialize(CEditorCore* _pCore, CScene* _pScene)
{
    m_pEditorCore = _pCore;
    m_pScene = _pScene;

    // 화면 해상도 가져오기
    RECT screenRect;
    GetClientRect(CCore::GetInst()->GetMainHwnd(), &screenRect);
    int screenWidth = screenRect.right - screenRect.left;

    // 오브젝트 팔레트 레이아웃 설정 - 오른쪽에 완전히 붙이기
    m_iPaletteWidth = 280;           // 패널 너비
    m_iPaletteHeight = 600;          // 패널 높이
    m_iPaletteX = screenWidth - m_iPaletteWidth - 10;  // 화면 오른쪽 끝에서 10px 떨어진 위치
    m_iPaletteY = 60;                // 툴바 아래
    m_iItemSize = 60;                // 아이템 크기
    m_iItemPadding = 8;              // 아이템 간격
    m_iItemsPerRow = 4;              // 한 줄에 4개씩
    m_iScrollOffset = 0;

    CalculatePaletteLayout();
}

void CEditorUI::Render(HDC _dc)
{
    if (!m_pEditorCore->IsShowUI())
        return;

    //RenderMainUI(_dc);
    RenderObjectPalette(_dc);
}

void CEditorUI::RenderMainUI(HDC _dc)
{
    // 툴바 높이만큼 아래로 이동한 위치에서 시작
    int toolbarHeight = m_pEditorCore->GetToolbar()->GetToolbarHeight();
    int adjustedUITop = m_iUIMargin + toolbarHeight;

    // UI 패널 배경 그리기
    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, m_iUIMargin, adjustedUITop, m_iUIWidth, m_iUIHeight + adjustedUITop);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 테두리
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);
    Rectangle(_dc, m_iUIMargin, adjustedUITop, m_iUIWidth, m_iUIHeight + adjustedUITop);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush2);
    DeleteObject(hPen);

    // 텍스트 기본 설정
    SetBkMode(_dc, TRANSPARENT);

    int yPos = adjustedUITop + 20; // 툴바 아래에서 시작

    // UI 섹션별 렌더링
    RenderUIHeader(_dc, yPos);
    RenderModeInfo(_dc, yPos);
    RenderBackgroundSettings(_dc, yPos);
    RenderObjectInfo(_dc, yPos);
    RenderTileVisualSettings(_dc, yPos);
    RenderGridInfo(_dc, yPos);
    RenderLevelBounds(_dc, yPos);
    RenderObjectCount(_dc, yPos);
    RenderControlInstructions(_dc, yPos);
    RenderBackgroundModeUI(_dc, yPos);
}

void CEditorUI::RenderUIHeader(HDC _dc, int& yPos)
{
    // 제목
    HFONT hFont = CreateUIFont(16, true);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    RenderBoldText(_dc, 20, yPos, L"=== KIRBY LEVEL EDITOR ===", RGB(255, 255, 100));

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
    yPos += 25;
}

void CEditorUI::RenderModeInfo(HDC _dc, int& yPos)
{
    // 현재 모드 표시
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Mode: %s", m_pEditorCore->GetModeString());
    RenderBoldText(_dc, 20, yPos, szBuffer, RGB(100, 255, 100));
    yPos += m_iLineHeight + 5;
}

void CEditorUI::RenderBackgroundSettings(HDC _dc, int& yPos)
{
    yPos += 5;
    RenderBoldText(_dc, 20, yPos, L"Background Settings:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    // 현재 배경 표시
    wchar_t szBgBuffer[256];
    const wchar_t* szCurrentBgName = m_pEditorCore->GetObjectManager()->GetBackgroundName(
        m_pEditorCore->GetObjectManager()->GetCurrentBackgroundType());
    swprintf_s(szBgBuffer, L"Current: %s", szCurrentBgName);
    RenderText(_dc, 20, yPos, szBgBuffer);
    yPos += m_iLineHeight;

    // 배경 변경 단축키 안내
    RenderText(_dc, 20, yPos, L"B - Background Mode", RGB(200, 200, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"Q / E - Change Background", RGB(200, 200, 255));
    yPos += m_iLineHeight + 5;
}

void CEditorUI::RenderObjectInfo(HDC _dc, int& yPos)
{
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode();

    // 현재 선택된 오브젝트 정보
    if (eMode == EDITOR_MODE::PLACE_MONSTER ||
        eMode == EDITOR_MODE::PLACE_ITEM ||
        eMode == EDITOR_MODE::PLACE_TILE ||
        eMode == EDITOR_MODE::PLACE_SPECIAL)
    {
        RenderBoldText(_dc, 20, yPos, L"Selected Object:", RGB(255, 255, 100));
        yPos += m_iLineHeight;

        wchar_t szBuffer[256];
        const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager()->GetCurrentCategory();
        swprintf_s(szBuffer, L"%s (%d/%d)",
            m_pEditorCore->GetObjectManager()->GetCurrentObjectName(),
            m_pEditorCore->GetObjectManager()->GetCurrentSubType() + 1,
            (int)vecCategory.size());
        RenderText(_dc, 20, yPos, szBuffer);
        yPos += m_iLineHeight + 5;
    }
}

void CEditorUI::RenderTileVisualSettings(HDC _dc, int& yPos)
{
    // 타일 모드일 때만 표시
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
    {
        RenderBoldText(_dc, 20, yPos, L"Tile Visual Settings:", RGB(255, 255, 100));
        yPos += m_iLineHeight;

        // 현재 타일 시각 타입 표시
        wchar_t szTileBuffer[256];
        const wchar_t* szCurrentTileName = m_pEditorCore->GetObjectManager()->GetTileVisualName(
            m_pEditorCore->GetObjectManager()->GetCurrentTileVisual());
        swprintf_s(szTileBuffer, L"Visual: %s", szCurrentTileName);
        RenderText(_dc, 20, yPos, szTileBuffer);
        yPos += m_iLineHeight;

        // 타일 변경 단축키 안내
        RenderText(_dc, 20, yPos, L"Q / E - Change Tile Visual", RGB(200, 200, 255));
        yPos += m_iLineHeight + 5;
    }
}

void CEditorUI::RenderGridInfo(HDC _dc, int& yPos)
{
    RenderBoldText(_dc, 20, yPos, L"Grid Settings:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    wchar_t szBuffer[256];

    swprintf_s(szBuffer, L"Size: %.0fpx", CGrid::GetInst()->GetGridSize());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Show: %s", CGrid::GetInst()->IsShowGrid() ? L"ON" : L"OFF");
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Snap: %s", CGrid::GetInst()->IsSnapToGrid() ? L"ON" : L"OFF");
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight + 5;
}

void CEditorUI::RenderObjectCount(HDC _dc, int& yPos)
{
    RenderBoldText(_dc, 20, yPos, L"Object Count:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    wchar_t szBuffer[256];

    const vector<CObject*>& vecPlayer = m_pScene->GetGroupObject(GROUP_TYPE::PLAYER);
    const vector<CObject*>& vecMonster = m_pScene->GetGroupObject(GROUP_TYPE::MONSTER);
    const vector<CObject*>& vecItem = m_pScene->GetGroupObject(GROUP_TYPE::ITEM);
    const vector<CObject*>& vecTile = m_pScene->GetGroupObject(GROUP_TYPE::TILE);
    const vector<CObject*>& vecSpecial = m_pScene->GetGroupObject(GROUP_TYPE::SPECIAL);

    swprintf_s(szBuffer, L"Players: %d", (int)vecPlayer.size());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Monsters: %d", (int)vecMonster.size());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Items: %d", (int)vecItem.size());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Tiles: %d", (int)vecTile.size());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight;

    swprintf_s(szBuffer, L"Special: %d", (int)vecSpecial.size());
    RenderText(_dc, 20, yPos, szBuffer);
    yPos += m_iLineHeight + 10;
}

void CEditorUI::RenderControlInstructions(HDC _dc, int& yPos)
{
    // 구분선
    RenderSeparatorLine(_dc, yPos);
    yPos += 10;

    // 컨트롤 안내
    RenderBoldText(_dc, 20, yPos, L"Object Placement:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"M - Monster Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"I - Item Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"T - Tile Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"P - Special Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"B - Background Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"R - Player Spawn Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;

    yPos += 3;
    RenderText(_dc, 20, yPos, L"Tab - Next Object", RGB(200, 200, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"Shift+Tab - Prev Object", RGB(200, 200, 255));
    yPos += m_iLineHeight;

    yPos += 5;
    RenderBoldText(_dc, 20, yPos, L"Edit Tools:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"S - Select Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"E - Erase Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"ESC - Normal Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;

    yPos += 5;
    RenderBoldText(_dc, 20, yPos, L"File & Navigation:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"F - Quick Save", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"L - Quick Load", RGB(255, 255, 255));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"Ctrl+S - Save As...", RGB(200, 255, 200));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"Ctrl+O - Open File...", RGB(200, 255, 200));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"Ctrl+T - Game Mode", RGB(255, 255, 255));
    yPos += m_iLineHeight;

    yPos += 5;
    RenderBoldText(_dc, 20, yPos, L"View Controls:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    RenderText(_dc, 20, yPos, L"G - Toggle Grid", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"1,2,3,4 - Grid Size", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"Arrow Keys - Camera", RGB(255, 255, 255));
    yPos += m_iLineHeight;
    RenderText(_dc, 20, yPos, L"H - Toggle UI", RGB(255, 255, 255));
    yPos += m_iLineHeight;
}

void CEditorUI::RenderBackgroundModeUI(HDC _dc, int& yPos)
{
    // 배경 모드일 때 특별한 UI 표시
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::BACKGROUND)
    {
        yPos += 10;

        // 배경 모드 전용 하이라이트 박스
        RenderHighlightBox(_dc, 15, yPos - 5, 420, 80);

        RenderBoldText(_dc, 20, yPos, L"=== BACKGROUND MODE ===", RGB(255, 255, 100));
        yPos += m_iLineHeight + 5;

        RenderText(_dc, 20, yPos, L"Available Backgrounds:", RGB(255, 255, 255));
        yPos += m_iLineHeight;

        // 사용 가능한 배경 목록 표시
        const vector<BACKGROUND_TYPE>& vecBgTypes = m_pEditorCore->GetObjectManager()->GetBackgroundTypes();
        BACKGROUND_TYPE eCurrentBg = m_pEditorCore->GetObjectManager()->GetCurrentBackgroundType();

        for (size_t i = 0; i < vecBgTypes.size(); ++i)
        {
            const wchar_t* szBgName = m_pEditorCore->GetObjectManager()->GetBackgroundName(vecBgTypes[i]);

            // 현재 선택된 배경은 하이라이트
            if (vecBgTypes[i] == eCurrentBg)
            {
                wchar_t szSelectedBg[256];
                swprintf_s(szSelectedBg, L"-> %s (Selected)", szBgName);
                RenderBoldText(_dc, 25, yPos, szSelectedBg, RGB(255, 255, 100));
            }
            else
            {
                wchar_t szBgEntry[256];
                swprintf_s(szBgEntry, L"   %s", szBgName);
                RenderText(_dc, 25, yPos, szBgEntry, RGB(200, 200, 200));
            }
            yPos += m_iLineHeight;
        }

        RenderText(_dc, 20, yPos, L"Click or Q/E to change", RGB(200, 200, 255));
    }
}

// CEditorUI.cpp에 추가
void CEditorUI::RenderLevelBounds(HDC _dc, int& yPos)
{
    RenderBoldText(_dc, 20, yPos, L"Level Bounds:", RGB(255, 255, 100));
    yPos += m_iLineHeight;

    CEditorCameraController* pCamera = m_pEditorCore->GetCameraController();
    if (pCamera)
    {
        Vec2 vMin = pCamera->GetCameraBoundsMin();
        Vec2 vMax = pCamera->GetCameraBoundsMax();

        // UI용 좌표 변환: Y축만 뒤집기
        float uiMinY = vMax.y - vMax.y;  // 실제 최상단 → UI 최상단 (0)
        float uiMaxY = vMax.y - vMin.y;  // 실제 최하단 → UI 최하단 (height)

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"X: %.0f ~ %.0f", vMin.x, vMax.x);
        RenderText(_dc, 20, yPos, szBuffer);
        yPos += m_iLineHeight;

        swprintf_s(szBuffer, L"Y: %.0f ~ %.0f", uiMinY, uiMaxY);
        RenderText(_dc, 20, yPos, szBuffer);
        yPos += m_iLineHeight;
    }
    else
    {
        RenderText(_dc, 20, yPos, L"No bounds set", RGB(200, 100, 100));
        yPos += m_iLineHeight;
    }

    yPos += 5;
}

void CEditorUI::DrawUIBackground(HDC _dc)
{
    // UI 패널 배경
    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, m_iUIMargin, m_iUIMargin, m_iUIWidth, m_iUIHeight);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 테두리
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);
    Rectangle(_dc, m_iUIMargin, m_iUIMargin, m_iUIWidth, m_iUIHeight);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush2);
    DeleteObject(hPen);
}

void CEditorUI::SetupTextStyle(HDC _dc, COLORREF color)
{
    SetTextColor(_dc, color);
    SetBkMode(_dc, TRANSPARENT);
}

void CEditorUI::RenderText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color)
{
    SetTextColor(_dc, color);
    TextOut(_dc, x, y, text, (int)wcslen(text));
}

void CEditorUI::RenderBoldText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color)
{
    HFONT hFont = CreateUIFont(14, true);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    SetTextColor(_dc, color);
    TextOut(_dc, x, y, text, (int)wcslen(text));

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
}

int CEditorUI::GetPaletteItemAt(Vec2 vMousePos)
{
    int startY = m_iPaletteY + 50;
    int relativeX = (int)vMousePos.x - (m_iPaletteX + 10);
    int relativeY = (int)vMousePos.y - startY + m_iScrollOffset;

    if (relativeX < 0 || relativeY < 0)
        return -1;

    int col = relativeX / (m_iItemSize + m_iItemPadding);
    int row = relativeY / (m_iItemSize + m_iItemPadding);

    if (col >= m_iItemsPerRow)
        return -1;

    int itemIndex = row * m_iItemsPerRow + col;
    return itemIndex;
}

bool CEditorUI::IsInPaletteArea(Vec2 vMousePos)
{
    return vMousePos.x >= m_iPaletteX && vMousePos.x <= m_iPaletteX + m_iPaletteWidth &&
        vMousePos.y >= m_iPaletteY && vMousePos.y <= m_iPaletteY + m_iPaletteHeight;
}

void CEditorUI::CalculatePaletteLayout()
{
    // 스크롤 최대값 계산 등 레이아웃 관련 계산
    const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager()->GetCurrentCategory();
    int totalRows = ((int)vecCategory.size() + m_iItemsPerRow - 1) / m_iItemsPerRow;
    int visibleRows = (m_iPaletteHeight - 60) / (m_iItemSize + m_iItemPadding);
    m_iMaxScroll = max(0, (totalRows - visibleRows) * (m_iItemSize + m_iItemPadding));
}

void CEditorUI::RenderSeparatorLine(HDC _dc, int yPos)
{
    HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN hOldLinePen = (HPEN)SelectObject(_dc, hLinePen);
    MoveToEx(_dc, 20, yPos, nullptr);
    LineTo(_dc, 430, yPos);
    SelectObject(_dc, hOldLinePen);
    DeleteObject(hLinePen);
}

void CEditorUI::RenderHighlightBox(HDC _dc, int x, int y, int width, int height)
{
    HBRUSH hHighlightBrush = CreateSolidBrush(RGB(50, 50, 100));
    HBRUSH hOldHighlightBrush = (HBRUSH)SelectObject(_dc, hHighlightBrush);
    Rectangle(_dc, x, y, x + width, y + height);
    SelectObject(_dc, hOldHighlightBrush);
    DeleteObject(hHighlightBrush);
}

void CEditorUI::RenderObjectPalette(HDC _dc)
{
    // 배치 모드가 아니면 팔레트 표시 안함
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode();
    if (eMode != EDITOR_MODE::PLACE_MONSTER &&
        eMode != EDITOR_MODE::PLACE_ITEM &&
        eMode != EDITOR_MODE::PLACE_TILE &&
        eMode != EDITOR_MODE::PLACE_SPECIAL)
    {
        return;
    }

    RenderPaletteBackground(_dc);
    RenderPaletteHeader(_dc);
    RenderPaletteItems(_dc);
}

bool CEditorUI::HandlePaletteClick(Vec2 vMousePos)
{
    if (!IsInPaletteArea(vMousePos))
        return false;

    EDITOR_MODE currentMode = m_pEditorCore->GetCurrentMode();

    // Stage Image 모드인 경우 별도 처리
    if (currentMode == EDITOR_MODE::PLACE_STAGE)
    {
        return HandleStageImagePaletteClick(vMousePos);
    }

    // 기존 객체 팔레트 클릭 처리
    int clickedIndex = GetPaletteItemAt(vMousePos);
    if (clickedIndex >= 0)
    {
        m_pEditorCore->GetObjectManager()->SetCurrentSubType(clickedIndex);
        return true;
    }

    return false;
}

bool CEditorUI::HandleStageImagePaletteClick(Vec2 vMousePos)
{
    // 사용 가능한 스테이지 이미지 타입들 가져오기
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.empty())
        return false;

    // 클릭된 항목 계산
    int relativeX = (int)vMousePos.x - (m_iPaletteX + 10);
    int relativeY = (int)vMousePos.y - (m_iPaletteY + 50) + m_iScrollOffset;

    if (relativeX < 0 || relativeY < 0)
        return false;

    int col = relativeX / (m_iItemSize + m_iItemPadding);
    int row = relativeY / (m_iItemSize + m_iItemPadding);
    int clickedIndex = row * m_iItemsPerRow + col;

    // 유효한 인덱스인지 확인
    if (clickedIndex >= 0 && clickedIndex < (int)availableTypes.size())
    {
        STAGE_IMAGE_TYPE selectedType = availableTypes[clickedIndex];
        CStageMgr::GetInst()->SetCurrentStageImage(selectedType);

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Stage Image selected: %s",
            CStageMgr::GetInst()->GetStageImageName(selectedType));
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);

        return true;
    }

    return false;
}

void CEditorUI::RenderPaletteBackground(HDC _dc)
{
    // 팔레트 배경
    HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, m_iPaletteX, m_iPaletteY,
        m_iPaletteX + m_iPaletteWidth, m_iPaletteY + m_iPaletteHeight);

    // 테두리
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    SelectObject(_dc, hHollowBrush);
    Rectangle(_dc, m_iPaletteX, m_iPaletteY,
        m_iPaletteX + m_iPaletteWidth, m_iPaletteY + m_iPaletteHeight);

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void CEditorUI::RenderPaletteHeader(HDC _dc)
{
    // 헤더 배경
    HBRUSH hHeaderBrush = CreateSolidBrush(RGB(80, 80, 80));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hHeaderBrush);
    Rectangle(_dc, m_iPaletteX, m_iPaletteY, m_iPaletteX + m_iPaletteWidth, m_iPaletteY + 40);

    // 헤더 텍스트
    HFONT hFont = CreateUIFont(16, true);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);
    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    const wchar_t* szTitle;
    EDITOR_MODE currentMode = m_pEditorCore->GetCurrentMode();

    switch (currentMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
        szTitle = L"Monsters";
        break;
    case EDITOR_MODE::PLACE_ITEM:
        szTitle = L"Items";
        break;
    case EDITOR_MODE::PLACE_TILE:
        szTitle = L"Tiles";
        break;
    case EDITOR_MODE::PLACE_SPECIAL:
        szTitle = L"Special Objects";
        break;
    case EDITOR_MODE::PLACE_STAGE:      // 새로 추가
        szTitle = L"Stage Images";
        break;
    default:
        szTitle = L"Objects";
        break;
    }

    TextOut(_dc, m_iPaletteX + 10, m_iPaletteY + 12, szTitle, (int)wcslen(szTitle));

    // Stage Image 모드인 경우 현재 스테이지 정보 표시
    if (currentMode == EDITOR_MODE::PLACE_STAGE)
    {
        STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();
        const wchar_t* currentStageName = CStageMgr::GetInst()->GetStageImageName(currentType);

        wchar_t szInfo[256];
        swprintf_s(szInfo, L"Current: %s", currentStageName);
        TextOut(_dc, m_iPaletteX + 10, m_iPaletteY + 28, szInfo, (int)wcslen(szInfo));
    }
    else
    {
        // 다른 모드에서는 기존 정보 표시
        const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager()->GetCurrentCategory();
        wchar_t szInfo[256];
        swprintf_s(szInfo, L"(%d/%d)",
            m_pEditorCore->GetObjectManager()->GetCurrentSubType() + 1,
            (int)vecCategory.size());
        TextOut(_dc, m_iPaletteX + 200, m_iPaletteY + 12, szInfo, (int)wcslen(szInfo));
    }

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldFont);
    DeleteObject(hHeaderBrush);
    DeleteObject(hFont);
}

void CEditorUI::RenderPaletteItems(HDC _dc)
{
    // 현재 카테고리의 객체들 가져오기
    const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager()->GetCurrentCategory();
    if (vecCategory.empty())
        return;

    // 현재 선택된 서브 타입
    int currentSubType = m_pEditorCore->GetObjectManager()->GetCurrentSubType();

    // 아이템들 렌더링 시작 위치
    int startX = m_iPaletteX + 10;
    int startY = m_iPaletteY + 50; // 헤더 아래

    // 스크롤 오프셋 적용
    int renderY = startY - m_iScrollOffset;

    for (size_t i = 0; i < vecCategory.size(); ++i)
    {
        // 그리드 위치 계산
        int col = i % m_iItemsPerRow;
        int row = i / m_iItemsPerRow;

        int itemX = startX + col * (m_iItemSize + m_iItemPadding);
        int itemY = renderY + row * (m_iItemSize + m_iItemPadding);

        // 화면 밖이면 스킵 (최적화)
        if (itemY + m_iItemSize < m_iPaletteY + 50 ||
            itemY > m_iPaletteY + m_iPaletteHeight)
        {
            continue;
        }

        // 선택 여부 확인
        bool isSelected = (i == currentSubType);

        // 아이템 렌더링
        RenderPaletteItem(_dc, (int)i, vecCategory[i], itemX, itemY, isSelected);
    }
}

void CEditorUI::RenderPaletteItem(HDC _dc, int index, OBJECT_TYPE objType, int x, int y, bool selected)
{
    // 아이템 배경
    COLORREF bgColor = selected ? RGB(100, 150, 100) : RGB(60, 60, 60);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, x, y, x + m_iItemSize, y + m_iItemSize);

    // 테두리
    COLORREF borderColor = selected ? RGB(150, 200, 150) : RGB(100, 100, 100);
    HPEN hPen = CreatePen(PS_SOLID, selected ? 3 : 1, borderColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    SelectObject(_dc, hHollowBrush);
    Rectangle(_dc, x, y, x + m_iItemSize, y + m_iItemSize);

    // 객체 아이콘 렌더링
    RenderObjectIcon(_dc, objType, x + 5, y + 5, m_iItemSize - 10);

    // 객체 이름 표시 (개선됨)
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(255, 255, 255));
    HFONT hFont = CreateUIFont(8, false);  // 더 작은 폰트 사용
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    // 타일/충돌체 타입인지 확인
    bool bIsTileType = (objType >= OBJECT_TYPE::TILE_GROUND && objType <= OBJECT_TYPE::TILE_INVISIBLE);

    wchar_t szDisplayName[32];
    if (bIsTileType)
    {
        // 충돌체 타입은 충돌체 이름 사용
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType(objType);
        const wchar_t* szCollisionName = CObjectFactory::GetCollisionTypeName(collisionType);

        // 이름을 줄여서 표시
        if (wcslen(szCollisionName) > 10)
        {
            wcsncpy_s(szDisplayName, szCollisionName, 8);
            szDisplayName[8] = L'.';
            szDisplayName[9] = L'.';
            szDisplayName[10] = L'\0';
        }
        else
        {
            wcscpy_s(szDisplayName, szCollisionName);
        }
    }
    else
    {
        // 기존 객체들은 기존 이름 사용
        const wchar_t* szName = CObjectFactory::GetObjectTypeName(objType);
        if (wcslen(szName) > 10)
        {
            wcsncpy_s(szDisplayName, szName, 8);
            szDisplayName[8] = L'.';
            szDisplayName[9] = L'.';
            szDisplayName[10] = L'\0';
        }
        else
        {
            wcscpy_s(szDisplayName, szName);
        }
    }

    // 텍스트를 아이템 하단에 표시 (2줄로 나누어서 표시)
    RECT textRect = { x + 2, y + m_iItemSize - 18, x + m_iItemSize - 2, y + m_iItemSize - 2 };
    DrawText(_dc, szDisplayName, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);

    // 충돌체인 경우 추가 정보 표시
    if (bIsTileType && selected)
    {
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType(objType);

        // 속성 표시 (작은 아이콘들)
        int iconY = y + m_iItemSize - 32;
        int iconX = x + 2;

        // 단단함 표시
        if (collisionType == COLLISION_TYPE::SOLID_GROUND ||
            collisionType == COLLISION_TYPE::SPIKE ||
            collisionType == COLLISION_TYPE::BREAKABLE_BLOCK ||
            collisionType == COLLISION_TYPE::INVISIBLE_WALL)
        {
            SetTextColor(_dc, RGB(100, 100, 255));
            TextOut(_dc, iconX, iconY, L"sol", 1);  // 단단함 표시
            iconX += 10;
        }

        // 데미지 표시
        if (collisionType == COLLISION_TYPE::SPIKE ||
            collisionType == COLLISION_TYPE::LAVA)
        {
            SetTextColor(_dc, RGB(255, 100, 100));
            TextOut(_dc, iconX, iconY, L"war", 1);  // 위험 표시
            iconX += 10;
        }

        // 일방통행 표시
        if (collisionType == COLLISION_TYPE::PLATFORM ||
            collisionType == COLLISION_TYPE::ONE_WAY_PLATFORM)
        {
            SetTextColor(_dc, RGB(100, 255, 100));
            TextOut(_dc, iconX, iconY, L"upp", 1);  // 위로만 충돌
        }
    }

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldFont);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    DeleteObject(hFont);
}

void CEditorUI::RenderObjectIcon(HDC _dc, OBJECT_TYPE objType, int x, int y, int size)
{
    // 객체 타입에 따른 간단한 아이콘 렌더링
    HBRUSH hIconBrush = nullptr;
    COLORREF iconColor = RGB(200, 200, 200); // 기본색

    // 타일/충돌체 타입인지 확인
    bool bIsTileType = (objType >= OBJECT_TYPE::TILE_GROUND && objType <= OBJECT_TYPE::TILE_INVISIBLE);

    if (bIsTileType)
    {
        // 타일/충돌체 타입은 색깔 박스로 표시
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType(objType);
        iconColor = CObjectFactory::GetCollisionTypeColor(collisionType);

        // 충돌체 박스 스타일로 렌더링
        hIconBrush = CreateSolidBrush(iconColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hIconBrush);

        // 사각형으로 그리기 (타일과 같은 모양)
        Rectangle(_dc, x + 8, y + 8, x + size - 8, y + size - 8);

        // 테두리 그리기 (더 진한 색으로)
        COLORREF borderColor = RGB(
            GetRValue(iconColor) / 2,
            GetGValue(iconColor) / 2,
            GetBValue(iconColor) / 2
        );

        HPEN hBorderPen = CreatePen(PS_SOLID, 2, borderColor);
        HPEN hOldPen = (HPEN)SelectObject(_dc, hBorderPen);
        HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);

        Rectangle(_dc, x + 8, y + 8, x + size - 8, y + size - 8);

        // 특수 표시 아이콘 추가
        RenderCollisionIcon(_dc, collisionType, x + size / 2, y + size / 2);

        SelectObject(_dc, hOldBrush);
        SelectObject(_dc, hOldPen);
        SelectObject(_dc, hOldBrush2);
        DeleteObject(hIconBrush);
        DeleteObject(hBorderPen);
    }
    else
    {
        // 기존 객체들은 원형 아이콘으로 표시
        switch (objType)
        {
        case OBJECT_TYPE::MONSTER_WADDLE_DEE:
            iconColor = RGB(255, 180, 100); // 주황색
            break;
        case OBJECT_TYPE::MONSTER_GORDOS:
            iconColor = RGB(128, 128, 128); // 회색 (가시)
            break;
        case OBJECT_TYPE::MONSTER_BRONTO_BURT:
            iconColor = RGB(100, 255, 255); // 하늘색
            break;
        case OBJECT_TYPE::MONSTER_HOT_HEAD:
            iconColor = RGB(255, 100, 100); // 빨간색
            break;
        case OBJECT_TYPE::ITEM_STAR:
            iconColor = RGB(255, 255, 100); // 노란색
            break;
        case OBJECT_TYPE::ITEM_ENERGY_DRINK:
            iconColor = RGB(100, 255, 100); // 초록색
            break;
        case OBJECT_TYPE::ITEM_1UP:
            iconColor = RGB(255, 100, 255); // 자주색
            break;
        case OBJECT_TYPE::ITEM_ABILITY_STAR:
            iconColor = RGB(100, 100, 255); // 파란색
            break;
        case OBJECT_TYPE::OBJECT_DOOR:
            iconColor = RGB(139, 69, 19);   // 갈색
            break;
        case OBJECT_TYPE::OBJECT_SWITCH:
            iconColor = RGB(255, 215, 0);   // 금색
            break;
        case OBJECT_TYPE::OBJECT_MIRROR:
            iconColor = RGB(192, 192, 192); // 은색
            break;
        default:
            iconColor = RGB(150, 150, 150);
            break;
        }

        hIconBrush = CreateSolidBrush(iconColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hIconBrush);

        // 간단한 도형으로 아이콘 표현
        Ellipse(_dc, x + 8, y + 8, x + size - 8, y + size - 8);

        SelectObject(_dc, hOldBrush);
        DeleteObject(hIconBrush);
    }
}

HFONT CEditorUI::CreateUIFont(int size, bool bold)
{
    return CreateFont(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
}

void CEditorUI::RenderCollisionIcon(HDC _dc, COLLISION_TYPE collisionType, int centerX, int centerY)
{
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(255, 255, 255));

    HFONT hFont = CreateUIFont(12, true);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    wchar_t szIcon[4] = L"";

    switch (collisionType)
    {
    case COLLISION_TYPE::SOLID_GROUND:
        wcscpy_s(szIcon, L"sol");  // 단단한 블록
        break;
    case COLLISION_TYPE::PLATFORM:
        wcscpy_s(szIcon, L"plf");  // 플랫폼
        break;
    case COLLISION_TYPE::SPIKE:
        wcscpy_s(szIcon, L"spk");  // 가시 (삼각형)
        break;
    case COLLISION_TYPE::WATER:
        wcscpy_s(szIcon, L"wat");  // 물결
        break;
    case COLLISION_TYPE::LAVA:
        wcscpy_s(szIcon, L"dia");  // 다이아몬드 (용암)
        break;
    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        wcscpy_s(szIcon, L"upp");  // 위쪽 화살표
        break;
    case COLLISION_TYPE::MOVING_PLATFORM:
        wcscpy_s(szIcon, L"lr");  // 좌우 화살표
        break;
    case COLLISION_TYPE::BREAKABLE_BLOCK:
        wcscpy_s(szIcon, L"emp");  // 빈 박스
        break;
    case COLLISION_TYPE::INVISIBLE_WALL:
        wcscpy_s(szIcon, L"?");  // 물음표
        break;
    default:
        wcscpy_s(szIcon, L"nor");  // 기본 블록
        break;
    }

    if (wcslen(szIcon) > 0)
    {
        // 텍스트 크기 측정
        SIZE textSize;
        GetTextExtentPoint32(_dc, szIcon, (int)wcslen(szIcon), &textSize);

        // 중앙에 배치
        TextOut(_dc,
            centerX - textSize.cx / 2,
            centerY - textSize.cy / 2,
            szIcon,
            (int)wcslen(szIcon));
    }

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
}

void CEditorUI::RenderStageImagePalette(HDC _dc)
{
    // 사용 가능한 스테이지 이미지 타입들 가져오기
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();

    // 팔레트 배경
    RenderPaletteBackground(_dc);
    RenderPaletteHeader(_dc);

    // 스테이지 이미지 항목들 렌더링
    int startX = m_iPaletteX + 10;
    int startY = m_iPaletteY + 50;
    int renderY = startY - m_iScrollOffset;

    for (size_t i = 0; i < availableTypes.size(); ++i)
    {
        STAGE_IMAGE_TYPE stageType = availableTypes[i];
        bool isSelected = (stageType == currentType);

        // 항목 위치 계산
        int col = i % m_iItemsPerRow;
        int row = i / m_iItemsPerRow;

        int itemX = startX + col * (m_iItemSize + m_iItemPadding);
        int itemY = renderY + row * (m_iItemSize + m_iItemPadding);

        // 화면 밖이면 스킵
        if (itemY + m_iItemSize < m_iPaletteY + 50 ||
            itemY > m_iPaletteY + m_iPaletteHeight)
        {
            continue;
        }

        // 스테이지 이미지 항목 렌더링
        RenderStageImageItem(_dc, stageType, itemX, itemY, isSelected);
    }
}

void CEditorUI::RenderStageImageItem(HDC _dc, STAGE_IMAGE_TYPE stageType, int x, int y, bool selected)
{
    // 아이템 배경
    COLORREF bgColor = selected ? RGB(100, 150, 100) : RGB(60, 60, 60);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, x, y, x + m_iItemSize, y + m_iItemSize);

    // 테두리
    COLORREF borderColor = selected ? RGB(150, 200, 150) : RGB(100, 100, 100);
    HPEN hPen = CreatePen(PS_SOLID, selected ? 2 : 1, borderColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    Rectangle(_dc, x, y, x + m_iItemSize, y + m_iItemSize);

    // 스테이지 타입에 따른 아이콘/미리보기
    int centerX = x + m_iItemSize / 2;
    int centerY = y + m_iItemSize / 2;

    // 실제 스테이지 이미지가 있으면 썸네일 표시, 없으면 아이콘
    CStageImage* pStageImage = CStageMgr::GetInst()->FindStageImage(stageType);
    if (pStageImage && pStageImage->GetStageTexture())
    {
        // 미니 썸네일 렌더링 (축소된 버전)
        CTexture* pTexture = pStageImage->GetStageTexture();

        int thumbnailSize = m_iItemSize - 10;
        int thumbnailX = x + 5;
        int thumbnailY = y + 5;

        // 텍스처를 축소해서 렌더링
        pTexture->RenderWithColorKey(_dc,
            Vec2((float)thumbnailX, (float)thumbnailY),
            Vec2((float)thumbnailSize, (float)thumbnailSize),
            RGB(255, 0, 255)); // 마젠타 컬러키
    }
    else
    {
        // 텍스처가 없으면 타입별 아이콘 표시
        RenderStageImageIcon(_dc, stageType, centerX, centerY);
    }

    // 스테이지 이름 표시 (아이템 하단)
    const wchar_t* stageName = CStageMgr::GetInst()->GetStageImageName(stageType);
    HFONT hFont = CreateUIFont(10);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);
    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    RECT textRect = { x, y + m_iItemSize - 20, x + m_iItemSize, y + m_iItemSize };
    DrawText(_dc, stageName, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(_dc, hOldFont);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hFont);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void CEditorUI::RenderStageImageIcon(HDC _dc, STAGE_IMAGE_TYPE stageType, int centerX, int centerY)
{
    COLORREF iconColor;
    const wchar_t* iconText;

    switch (stageType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:
        iconColor = RGB(100, 255, 100);  // 초록색 (Green Hill)
        iconText = L"S1";
        break;
    case STAGE_IMAGE_TYPE::STAGE_02:
        iconColor = RGB(150, 150, 150);  // 회색 (Castle)
        iconText = L"S2";
        break;
    case STAGE_IMAGE_TYPE::CUSTOM:
        iconColor = RGB(255, 255, 100);  // 노란색 (Custom)
        iconText = L"CU";
        break;
    default:
        iconColor = RGB(255, 255, 255);
        iconText = L"??";
        break;
    }

    // 아이콘 배경 원
    HBRUSH hBrush = CreateSolidBrush(iconColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Ellipse(_dc, centerX - 15, centerY - 15, centerX + 15, centerY + 15);

    // 아이콘 텍스트
    HFONT hFont = CreateUIFont(12, true);
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);
    SetTextColor(_dc, RGB(0, 0, 0));
    SetBkMode(_dc, TRANSPARENT);

    RECT textRect = { centerX - 15, centerY - 8, centerX + 15, centerY + 8 };
    DrawText(_dc, iconText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(_dc, hOldFont);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hFont);
    DeleteObject(hBrush);
}
