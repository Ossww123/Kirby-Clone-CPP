#include "pch.h"
#include "CEditorUI.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"

#include "CScene.h"
#include "CGrid.h"
#include "CBackgroundMgr.h"

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
}

void CEditorUI::Render(HDC _dc)
{
    if (!m_pEditorCore->IsShowUI())
        return;

    RenderMainUI(_dc);
}

void CEditorUI::RenderMainUI(HDC _dc)
{
    // UI 배경 그리기
    DrawUIBackground(_dc);

    // 텍스트 기본 설정
    SetBkMode(_dc, TRANSPARENT);

    int yPos = 20;

    // UI 섹션별 렌더링
    RenderUIHeader(_dc, yPos);
    RenderModeInfo(_dc, yPos);
    RenderBackgroundSettings(_dc, yPos);
    RenderObjectInfo(_dc, yPos);
    RenderTileVisualSettings(_dc, yPos);
    RenderGridInfo(_dc, yPos);
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

HFONT CEditorUI::CreateUIFont(int size, bool bold)
{
    return CreateFont(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
}