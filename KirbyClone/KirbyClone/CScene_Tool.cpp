#include "pch.h"
#include "CScene_Tool.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"
#include "CKeyMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CTimeMgr.h"
#include "CEventMgr.h"
#include "CPathMgr.h"
#include "CGrid.h"
#include "CTile.h"

CScene_Tool::CScene_Tool()
    : m_bShowUI(true)
    , m_vMousePos{}
    , m_bMouseClick(false)
    , m_fClickTime(0.f)
    , m_eCurrentMode(EDITOR_MODE::NONE)
    , m_pSelectedObject(nullptr)
    , m_bDragging(false)
    , m_vDragStartPos{}
    , m_eCurrentObjectType(OBJECT_TYPE::MONSTER_WADDLE_DEE)
    , m_iCurrentSubType(0)
    , m_vecCurrentCategory{}
{
}

CScene_Tool::~CScene_Tool()
{
}

void CScene_Tool::Enter()
{
    // 그리드 시스템 초기화
    CGrid::GetInst()->init();

    // 오브젝트 팩토리 기본 설정
    m_eCurrentObjectType = OBJECT_TYPE::MONSTER_WADDLE_DEE;
    ChangeObjectCategory(L"Monster"); // 기본 카테고리 설정

    // 깔끔한 빈 레벨로 시작 (임시 오브젝트들 제거)
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - Clean slate ready!");
}

void CScene_Tool::Exit()
{
    DeleteAllObject();
}

void CScene_Tool::Update()
{
    // 부모 클래스의 Update 호출 (모든 오브젝트 업데이트)
    CScene::Update();

    // 카메라 이동 처리 (항상 활성화)
    UpdateCameraMove();

    // 에디터 전용 입력 처리
    UpdateInput();
    UpdateMouse();

    // 오브젝트 선택 입력 처리
    UpdateObjectSelection();
}

void CScene_Tool::Render(HDC _dc)
{
    // 그리드 먼저 렌더링 (배경)
    CGrid::GetInst()->Render(_dc);

    // 부모 클래스의 Render 호출 (모든 오브젝트 렌더링)
    CScene::Render(_dc);

    // 선택된 오브젝트 하이라이트 (오브젝트 위에)
    RenderSelectedObject(_dc);

    // 배치 미리보기 렌더링 (마우스 커서보다 먼저)
    RenderPreview(_dc);

    // 마우스 커서 렌더링
    RenderMouse(_dc);

    // 에디터 UI 렌더링 (맨 위에 그려야 함)
    if (m_bShowUI)
    {
        RenderUI(_dc);
    }
}

void CScene_Tool::UpdateInput()
{
    // UI 토글 (H 키)
    if (KEY_TAP(KEY::H))
    {
        m_bShowUI = !m_bShowUI;

        if (m_bShowUI)
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - UI ON");
        else
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - UI OFF");
    }

    // 그리드 관련 입력
    UpdateGridInput();

    // 모드 전환 입력 처리
    UpdateModeInput();
}

void CScene_Tool::UpdateGridInput()
{
    // 그리드 표시 토글 (G 키)
    if (KEY_TAP(KEY::G))
    {
        bool bShowGrid = CGrid::GetInst()->IsShowGrid();
        CGrid::GetInst()->SetShowGrid(!bShowGrid);

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Grid %s", bShowGrid ? L"OFF" : L"ON");
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }

    // 그리드 스냅 토글 (Ctrl + G)
    if (KEY_HOLD(KEY::G) && KEY_TAP(KEY::G))
    {
        bool bSnapToGrid = CGrid::GetInst()->IsSnapToGrid();
        CGrid::GetInst()->SetSnapToGrid(!bSnapToGrid);

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Grid Snap %s", bSnapToGrid ? L"OFF" : L"ON");
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }

    // 그리드 크기 조절 (1, 2, 3, 4 키)
    if (KEY_TAP(KEY::ALPHA_1))
    {
        CGrid::GetInst()->SetGridSizePreset(1);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Grid Size: 32px");
    }
    else if (KEY_TAP(KEY::ALPHA_2))
    {
        CGrid::GetInst()->SetGridSizePreset(2);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Grid Size: 64px");
    }
    else if (KEY_TAP(KEY::ALPHA_3))
    {
        CGrid::GetInst()->SetGridSizePreset(3);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Grid Size: 128px");
    }
    else if (KEY_TAP(KEY::ALPHA_4))
    {
        CGrid::GetInst()->SetGridSizePreset(4);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Grid Size: 256px");
    }
}

void CScene_Tool::UpdateMouse()
{
    // CKeyMgr에서 마우스 좌표 가져오기
    m_vMousePos = CKeyMgr::GetInst()->GetMouseWorldPos();

    // 그리드 스냅 적용
    if (CGrid::GetInst()->IsSnapToGrid())
    {
        m_vMousePos = CGrid::GetInst()->SnapToGrid(m_vMousePos);
    }

    // 마우스 클릭 감지
    if (KEY_TAP(KEY::MOUSE_LEFT))
    {
        m_bMouseClick = true;
        m_fClickTime = 0.5f;

        HandleMouseClick();
    }

    // 마우스 버튼을 떼면 드래그 종료
    if (KEY_AWAY(KEY::MOUSE_LEFT))
    {
        m_bDragging = false;
    }

    // 드래그 중이면 선택된 오브젝트 이동 (그리드 스냅 적용)
    if (m_bDragging && m_pSelectedObject && m_eCurrentMode == EDITOR_MODE::SELECT)
    {
        m_pSelectedObject->SetPos(m_vMousePos);
    }

    // 클릭 표시 시간 감소
    if (m_fClickTime > 0.f)
    {
        m_fClickTime -= CTimeMgr::GetInst()->GetfDT();
        if (m_fClickTime <= 0.f)
        {
            m_bMouseClick = false;
        }
    }
}

void CScene_Tool::RenderMouse(HDC _dc)
{
    // 마우스 월드 좌표를 화면 좌표로 변환
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vMousePos);

    // 모드에 따른 커서 색상 설정
    COLORREF cursorColor = RGB(255, 255, 0); // 기본: 노란색
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
        cursorColor = RGB(100, 255, 100); // 녹색
        break;
    case EDITOR_MODE::SELECT:
        cursorColor = RGB(100, 200, 255); // 파란색
        break;
    case EDITOR_MODE::ERASE:
        cursorColor = RGB(255, 100, 100); // 빨간색
        break;
    case EDITOR_MODE::CAMERA_MOVE:
        cursorColor = RGB(255, 255, 100); // 노란색
        break;
    }

    // 마우스 커서 그리기 (십자가)
    HPEN hPen = CreatePen(PS_SOLID, 2, cursorColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    int size = 8;
    // 가로선
    MoveToEx(_dc, (int)vRenderPos.x - size, (int)vRenderPos.y, nullptr);
    LineTo(_dc, (int)vRenderPos.x + size, (int)vRenderPos.y);
    // 세로선
    MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y - size, nullptr);
    LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y + size);

    // 그리드 스냅이 활성화된 경우 그리드 위치 표시
    if (CGrid::GetInst()->IsSnapToGrid())
    {
        HPEN hGridPen = CreatePen(PS_DOT, 1, cursorColor);
        SelectObject(_dc, hGridPen);

        float fGridSize = CGrid::GetInst()->GetGridSize();
        Vec2 vGridRenderPos = CCamera::GetInst()->GetRenderPos(m_vMousePos);

        // 그리드 셀 경계 표시
        Rectangle(_dc,
            (int)(vGridRenderPos.x - fGridSize / 2.f),
            (int)(vGridRenderPos.y - fGridSize / 2.f),
            (int)(vGridRenderPos.x + fGridSize / 2.f),
            (int)(vGridRenderPos.y + fGridSize / 2.f));

        DeleteObject(hGridPen);
    }

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);

    // 클릭했을 때 원 그리기
    if (m_bMouseClick)
    {
        HPEN hClickPen = CreatePen(PS_SOLID, 3, cursorColor);
        HPEN hOldClickPen = (HPEN)SelectObject(_dc, hClickPen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

        Ellipse(_dc,
            (int)vRenderPos.x - 15, (int)vRenderPos.y - 15,
            (int)vRenderPos.x + 15, (int)vRenderPos.y + 15);

        SelectObject(_dc, hOldClickPen);
        SelectObject(_dc, hOldBrush);
        DeleteObject(hClickPen);
    }
}

void CScene_Tool::RenderUI(HDC _dc)
{
    // UI 배경 패널
    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);
    Rectangle(_dc, 10, 10, 450, 800); // 패널 크기 확장
    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 테두리
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);
    Rectangle(_dc, 10, 10, 450, 800);
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush2);
    DeleteObject(hPen);

    // 텍스트 설정
    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    int yPos = 20;
    int lineHeight = 18;

    // 제목
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    TextOut(_dc, 20, yPos, L"=== KIRBY LEVEL EDITOR ===", 26);

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
    yPos += 25;

    // 현재 모드 표시
    SetTextColor(_dc, RGB(100, 255, 100));
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Mode: %s", GetModeString());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight + 5;

    // 현재 선택된 오브젝트 정보
    if (m_eCurrentMode == EDITOR_MODE::PLACE_MONSTER ||
        m_eCurrentMode == EDITOR_MODE::PLACE_ITEM ||
        m_eCurrentMode == EDITOR_MODE::PLACE_TILE ||
        m_eCurrentMode == EDITOR_MODE::PLACE_SPECIAL)
    {
        SetTextColor(_dc, RGB(255, 255, 100));
        TextOut(_dc, 20, yPos, L"Selected Object:", 16);
        yPos += lineHeight;

        SetTextColor(_dc, RGB(255, 255, 255));
        swprintf_s(szBuffer, L"%s (%d/%d)", GetCurrentObjectName(),
            m_iCurrentSubType + 1, (int)m_vecCurrentCategory.size());
        TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
        yPos += lineHeight + 5;
    }

    // 그리드 정보
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"Grid Settings:", 14);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Size: %.0fpx", CGrid::GetInst()->GetGridSize());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Show: %s", CGrid::GetInst()->IsShowGrid() ? L"ON" : L"OFF");
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Snap: %s", CGrid::GetInst()->IsSnapToGrid() ? L"ON" : L"OFF");
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight + 5;

    // 오브젝트 개수 정보
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"Object Count:", 13);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    const vector<CObject*>& vecMonster = GetGroupObject(GROUP_TYPE::MONSTER);
    const vector<CObject*>& vecItem = GetGroupObject(GROUP_TYPE::ITEM);
    const vector<CObject*>& vecTile = GetGroupObject(GROUP_TYPE::TILE);
    const vector<CObject*>& vecSpecial = GetGroupObject(GROUP_TYPE::SPECIAL);

    swprintf_s(szBuffer, L"Players: %d", (int)vecPlayer.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Monsters: %d", (int)vecMonster.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Items: %d", (int)vecItem.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Tiles: %d", (int)vecTile.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Special: %d", (int)vecSpecial.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight + 10;

    // 구분선
    HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN hOldLinePen = (HPEN)SelectObject(_dc, hLinePen);
    MoveToEx(_dc, 20, yPos, nullptr);
    LineTo(_dc, 430, yPos);
    SelectObject(_dc, hOldLinePen);
    DeleteObject(hLinePen);
    yPos += 10;

    // 컨트롤 안내
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"Object Placement:", 17);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    TextOut(_dc, 20, yPos, L"M - Monster Mode", 16);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"I - Item Mode", 13);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"T - Tile Mode", 13);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"P - Special Mode", 16);
    yPos += lineHeight;

    yPos += 3;
    SetTextColor(_dc, RGB(200, 200, 255));
    TextOut(_dc, 20, yPos, L"Tab - Next Object", 17);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Shift+Tab - Prev Object", 23);
    yPos += lineHeight;

    yPos += 5;
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"Edit Tools:", 11);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    TextOut(_dc, 20, yPos, L"S - Select Mode", 15);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"E - Erase Mode", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"ESC - Normal Mode", 17);
    yPos += lineHeight;

    yPos += 5;
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"File & Navigation:", 18);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    TextOut(_dc, 20, yPos, L"F - Quick Save", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"L - Quick Load", 14);
    yPos += lineHeight;
    
    SetTextColor(_dc, RGB(200, 255, 200));
    TextOut(_dc, 20, yPos, L"Ctrl+S - Save As...", 19);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Ctrl+O - Open File...", 21);
    yPos += lineHeight;

    SetTextColor(_dc, RGB(255, 255, 255));
    TextOut(_dc, 20, yPos, L"Ctrl+T - Game Mode", 18);
    yPos += lineHeight;

    yPos += 5;
    SetTextColor(_dc, RGB(255, 255, 100));
    TextOut(_dc, 20, yPos, L"View Controls:", 14);
    SetTextColor(_dc, RGB(255, 255, 255));
    yPos += lineHeight;

    TextOut(_dc, 20, yPos, L"G - Toggle Grid", 15);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"1,2,3,4 - Grid Size", 19);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Arrow Keys - Camera", 19);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"H - Toggle UI", 13);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Ctrl+T - Return to Game", 23);
}

void CScene_Tool::UpdateModeInput()
{
    // === 파일 관리 (Ctrl 조합키를 먼저 체크) ===
    // Ctrl+S키: 다른 이름으로 저장 (단순 S키보다 먼저 체크)
    if (KEY_TAP(KEY::S) && KEY_HOLD(KEY::CTRL))
    {
        SaveAsDialog();
    }
    // Ctrl+O키: 파일 열기
    else if (KEY_TAP(KEY::O) && KEY_HOLD(KEY::CTRL))
    {
        OpenDialog();
    }

    // === 오브젝트 배치 모드들 ===
    // M키: 몬스터 배치 모드 (Monster)
    else if (KEY_TAP(KEY::M))
    {
        ChangeMode(EDITOR_MODE::PLACE_MONSTER);
        ChangeObjectCategory(L"Monster");
    }
    // I키: 아이템 배치 모드 (Item)
    else if (KEY_TAP(KEY::I))
    {
        ChangeMode(EDITOR_MODE::PLACE_ITEM);
        ChangeObjectCategory(L"Item");
    }
    // T키: 타일 배치 모드 (Tile)
    else if (KEY_TAP(KEY::T))
    {
        ChangeMode(EDITOR_MODE::PLACE_TILE);
        ChangeObjectCategory(L"Tile");
    }
    // P키: 특수 오브젝트 배치 모드 (sPecial)
    else if (KEY_TAP(KEY::P))
    {
        ChangeMode(EDITOR_MODE::PLACE_SPECIAL);
        ChangeObjectCategory(L"Special");
    }

    // === 편집 모드들 ===
    // S키: 선택 모드 (Select) - Ctrl+S 체크 후에 배치
    else if (KEY_TAP(KEY::S))
    {
        ChangeMode(EDITOR_MODE::SELECT);
    }
    // E키: 삭제 모드 (Erase)
    else if (KEY_TAP(KEY::E))
    {
        ChangeMode(EDITOR_MODE::ERASE);
    }
    // ESC키: 기본 모드
    else if (KEY_TAP(KEY::ESC))
    {
        ChangeMode(EDITOR_MODE::NONE);
    }

    // === 기본 파일 관리 ===
    // F키: 빠른 저장 (File save)
    else if (KEY_TAP(KEY::F))
    {
        QuickSave();
    }
    // L키: 빠른 로드 (Load)
    else if (KEY_TAP(KEY::L))
    {
        QuickLoad();
    }

    // === 씬 전환 ===
    // Ctrl+T키: 게임으로 복귀
    // 실제 씬 전환은 CSceneMgr에서 처리됨
}

void CScene_Tool::SaveAsDialog()
{
    // 레벨 폴더 경로 확인 및 생성
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strLevelDir = strContentPath + L"level\\";
    CreateDirectory(strLevelDir.c_str(), nullptr);

    // 파일 대화상자 구조체 초기화
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };       // 파일명 버퍼
    wchar_t szFileTitle[260] = { 0 };  // 파일 제목 버퍼

    // 기본 파일명 설정
    wcscpy_s(szFile, L"NewLevel.lvl");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = CCore::GetInst()->GetMainHwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = strLevelDir.c_str();  // 초기 디렉토리를 level 폴더로 설정
    ofn.lpstrFilter = L"Kirby Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"레벨 파일 저장";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"lvl";

    // 저장 대화상자 표시
    if (GetSaveFileName(&ofn))
    {
        // 전체 경로에서 파일명만 추출
        wstring strFullPath = szFile;
        wstring strFileName = szFileTitle;

        // 확장자 제거 (SaveLevel 함수에서 자동으로 .lvl 추가)
        size_t dotPos = strFileName.rfind(L'.');
        if (dotPos != wstring::npos)
        {
            strFileName = strFileName.substr(0, dotPos);
        }

        // 레벨 저장
        SaveLevel(strFileName);

        // 성공 메시지
        wchar_t szMsg[512];
        swprintf_s(szMsg, L"레벨이 저장되었습니다: %s", strFileName.c_str());
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
    }
}

void CScene_Tool::OpenDialog()
{
    // 레벨 폴더 경로 확인
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strLevelDir = strContentPath + L"level\\";

    // 파일 대화상자 구조체 초기화
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };       // 파일명 버퍼
    wchar_t szFileTitle[260] = { 0 };  // 파일 제목 버퍼

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = CCore::GetInst()->GetMainHwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = strLevelDir.c_str();  // 초기 디렉토리를 level 폴더로 설정
    ofn.lpstrFilter = L"Kirby Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"레벨 파일 열기";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    // 열기 대화상자 표시
    if (GetOpenFileName(&ofn))
    {
        // 전체 경로에서 파일명만 추출
        wstring strFileName = szFileTitle;

        // 확장자 제거 (LoadLevel 함수에서 자동으로 .lvl 추가)
        size_t dotPos = strFileName.rfind(L'.');
        if (dotPos != wstring::npos)
        {
            strFileName = strFileName.substr(0, dotPos);
        }

        // 레벨 로드
        LoadLevel(strFileName);

        // 성공 메시지
        wchar_t szMsg[512];
        swprintf_s(szMsg, L"레벨이 로드되었습니다: %s", strFileName.c_str());
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
    }
}

void CScene_Tool::UpdateObjectSelection()
{
    // Tab키: 카테고리 내 다음 오브젝트
    if (KEY_TAP(KEY::TAB) && !KEY_HOLD(KEY::SHIFT))
    {
        NextObjectInCategory();
    }
    // Shift+Tab키: 카테고리 내 이전 오브젝트
    else if (KEY_TAP(KEY::TAB) && KEY_HOLD(KEY::SHIFT))
    {
        PrevObjectInCategory();
    }
}

void CScene_Tool::ChangeMode(EDITOR_MODE _eMode)
{
    m_eCurrentMode = _eMode;

    // 모드 변경 시 윈도우 타이틀 업데이트
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Level Editor - Mode: %s", GetModeString());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

const wchar_t* CScene_Tool::GetModeString()
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::NONE:         return L"Normal";
    case EDITOR_MODE::PLACE_MONSTER: return L"Place Monster (M)";
    case EDITOR_MODE::PLACE_ITEM:   return L"Place Item (I)";
    case EDITOR_MODE::PLACE_TILE:   return L"Place Tile (T)";
    case EDITOR_MODE::PLACE_SPECIAL: return L"Place Special (P)";
    case EDITOR_MODE::SELECT:       return L"Select (S)";
    case EDITOR_MODE::ERASE:        return L"Erase (E)";
    case EDITOR_MODE::CAMERA_MOVE:  return L"Camera Move";
    default:                        return L"Unknown";
    }
}

void CScene_Tool::HandleMouseClick()
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
    case EDITOR_MODE::PLACE_ITEM:
    case EDITOR_MODE::PLACE_TILE:
    case EDITOR_MODE::PLACE_SPECIAL:
        PlaceObject(m_vMousePos);
        break;

    case EDITOR_MODE::SELECT:
    {
        // 클릭한 위치에서 오브젝트 찾기
        CObject* pClickedObj = FindObjectAtPosition(m_vMousePos);
        if (pClickedObj)
        {
            SetSelectedObject(pClickedObj);
            m_bDragging = true;
            m_vDragStartPos = m_vMousePos;
        }
        else
        {
            // 빈 공간 클릭 시 선택 해제
            DeselectObject();
        }
    }
    break;

    case EDITOR_MODE::ERASE:
        DeleteObjectAtPosition(m_vMousePos);
        break;

    case EDITOR_MODE::NONE:
    case EDITOR_MODE::CAMERA_MOVE:
    default:
        // 기본 모드에서는 클릭 위치만 표시
        {
            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Clicked at: (%.0f, %.0f)", m_vMousePos.x, m_vMousePos.y);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
        }
        break;
    }
}

void CScene_Tool::PlaceObject(Vec2 _vPos)
{
    // 팩토리를 사용해서 오브젝트 생성
    CObject* pObject = CObjectFactory::CreateObject(m_eCurrentObjectType, _vPos);

    if (!pObject)
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Failed to create object!");
        return;
    }

    // 적절한 그룹에 추가
    GROUP_TYPE eGroup = CObjectFactory::GetObjectGroup(m_eCurrentObjectType);
    AddObject(pObject, eGroup);

    // 성공 메시지
    wchar_t szBuffer[256];
    const wchar_t* szObjectName = CObjectFactory::GetObjectTypeName(m_eCurrentObjectType);
    swprintf_s(szBuffer, L"%s placed at (%.0f, %.0f)!", szObjectName, _vPos.x, _vPos.y);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

void CScene_Tool::ChangeObjectCategory(const wstring& _strCategory)
{
    m_vecCurrentCategory = CObjectFactory::GetObjectTypesByCategory(_strCategory);

    if (!m_vecCurrentCategory.empty())
    {
        m_iCurrentSubType = 0;
        m_eCurrentObjectType = m_vecCurrentCategory[0];

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Category: %s - Object: %s",
            _strCategory.c_str(), GetCurrentObjectName());
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
}

void CScene_Tool::NextObjectInCategory()
{
    if (m_vecCurrentCategory.empty()) return;

    m_iCurrentSubType = (m_iCurrentSubType + 1) % m_vecCurrentCategory.size();
    m_eCurrentObjectType = m_vecCurrentCategory[m_iCurrentSubType];

    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Selected: %s (%d/%d)",
        GetCurrentObjectName(), m_iCurrentSubType + 1, (int)m_vecCurrentCategory.size());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

void CScene_Tool::PrevObjectInCategory()
{
    if (m_vecCurrentCategory.empty()) return;

    m_iCurrentSubType = (m_iCurrentSubType - 1 + m_vecCurrentCategory.size()) % m_vecCurrentCategory.size();
    m_eCurrentObjectType = m_vecCurrentCategory[m_iCurrentSubType];

    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Selected: %s (%d/%d)",
        GetCurrentObjectName(), m_iCurrentSubType + 1, (int)m_vecCurrentCategory.size());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

const wchar_t* CScene_Tool::GetCurrentObjectName()
{
    return CObjectFactory::GetObjectTypeName(m_eCurrentObjectType);
}

void CScene_Tool::RenderPreview(HDC _dc)
{
    // 배치 모드일 때 배치 미리보기
    if (m_eCurrentMode == EDITOR_MODE::PLACE_MONSTER ||
        m_eCurrentMode == EDITOR_MODE::PLACE_ITEM ||
        m_eCurrentMode == EDITOR_MODE::PLACE_TILE ||
        m_eCurrentMode == EDITOR_MODE::PLACE_SPECIAL)
    {
        Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vMousePos);
        Vec2 vObjectSize = CObjectFactory::GetDefaultScale(m_eCurrentObjectType);

        // 오브젝트 타입에 따른 색상 설정
        COLORREF previewColor = RGB(100, 255, 100); // 기본 초록색
        switch (m_eCurrentMode)
        {
        case EDITOR_MODE::PLACE_MONSTER:
            previewColor = RGB(255, 100, 100); // 빨간색
            break;
        case EDITOR_MODE::PLACE_ITEM:
            previewColor = RGB(255, 255, 100); // 노란색
            break;
        case EDITOR_MODE::PLACE_TILE:
            previewColor = RGB(100, 100, 255); // 파란색
            break;
        case EDITOR_MODE::PLACE_SPECIAL:
            previewColor = RGB(255, 100, 255); // 자주색
            break;
        }

        // 반투명 효과를 위한 펜과 브러시 설정
        HPEN hPen = CreatePen(PS_SOLID, 2, previewColor);
        HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
        HBRUSH hBrush = CreateHatchBrush(HS_DIAGCROSS, previewColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

        // 미리보기 사각형 그리기
        Rectangle(_dc,
            (int)(vRenderPos.x - vObjectSize.x / 2.f),
            (int)(vRenderPos.y - vObjectSize.y / 2.f),
            (int)(vRenderPos.x + vObjectSize.x / 2.f),
            (int)(vRenderPos.y + vObjectSize.y / 2.f));

        SelectObject(_dc, hOldPen);
        SelectObject(_dc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        // 오브젝트 이름 표시
        SetTextColor(_dc, previewColor);
        SetBkMode(_dc, TRANSPARENT);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

        const wchar_t* szObjectName = GetCurrentObjectName();
        int textX = (int)vRenderPos.x - (wcslen(szObjectName) * 3);
        int textY = (int)vRenderPos.y - (int)vObjectSize.y / 2 - 20;

        TextOut(_dc, textX, textY, szObjectName, wcslen(szObjectName));

        SelectObject(_dc, hOldFont);
        DeleteObject(hFont);
    }
    // 삭제 모드일 때 삭제 대상 표시
    else if (m_eCurrentMode == EDITOR_MODE::ERASE)
    {
        CObject* pTargetObj = FindObjectAtPosition(m_vMousePos);
        if (pTargetObj)
        {
            Vec2 vPos = pTargetObj->GetPos();
            Vec2 vScale = pTargetObj->GetScale();
            Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

            // 삭제 대상 표시 (빨간색 X표시)
            HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 100, 100)); // 빨간색
            HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

            // X 표시 그리기
            int halfSize = (int)(max(vScale.x, vScale.y) / 2.f + 10);
            MoveToEx(_dc, (int)vRenderPos.x - halfSize, (int)vRenderPos.y - halfSize, nullptr);
            LineTo(_dc, (int)vRenderPos.x + halfSize, (int)vRenderPos.y + halfSize);
            MoveToEx(_dc, (int)vRenderPos.x + halfSize, (int)vRenderPos.y - halfSize, nullptr);
            LineTo(_dc, (int)vRenderPos.x - halfSize, (int)vRenderPos.y + halfSize);

            SelectObject(_dc, hOldPen);
            DeleteObject(hPen);

            // "DELETE" 텍스트 표시
            SetTextColor(_dc, RGB(255, 100, 100));
            SetBkMode(_dc, TRANSPARENT);

            HFONT hFont = CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

            int textX = (int)vRenderPos.x - 20;
            int textY = (int)vRenderPos.y - halfSize - 20;

            TextOut(_dc, textX, textY, L"DELETE", 6);

            SelectObject(_dc, hOldFont);
            DeleteObject(hFont);
        }
    }
}

void CScene_Tool::UpdateCameraMove()
{
    // 카메라 이동 (화살표 키 사용)
    float fCameraSpeed = 500.f * CTimeMgr::GetInst()->GetfDT();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    if (KEY_HOLD(KEY::UP))      // W → UP
        vCameraPos.y -= fCameraSpeed;
    if (KEY_HOLD(KEY::DOWN))    // S → DOWN  
        vCameraPos.y += fCameraSpeed;
    if (KEY_HOLD(KEY::LEFT))    // A → LEFT
        vCameraPos.x -= fCameraSpeed;
    if (KEY_HOLD(KEY::RIGHT))   // D → RIGHT
        vCameraPos.x += fCameraSpeed;

    CCamera::GetInst()->SetLookAt(vCameraPos);
}

CObject* CScene_Tool::FindObjectAtPosition(Vec2 _vPos)
{
    // 모든 그룹에서 오브젝트 검색 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = GetGroupObject((GROUP_TYPE)i);

        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            Vec2 vObjPos = vecObj[j]->GetPos();
            Vec2 vObjScale = vecObj[j]->GetScale();

            // AABB 검사 (사각형 충돌 검사)
            if (_vPos.x >= vObjPos.x - vObjScale.x / 2.f &&
                _vPos.x <= vObjPos.x + vObjScale.x / 2.f &&
                _vPos.y >= vObjPos.y - vObjScale.y / 2.f &&
                _vPos.y <= vObjPos.y + vObjScale.y / 2.f)
            {
                return vecObj[j];
            }
        }
    }

    return nullptr;
}

void CScene_Tool::SetSelectedObject(CObject* _pObj)  // 함수 이름 변경
{
    m_pSelectedObject = _pObj;

    if (_pObj)
    {
        Vec2 vPos = _pObj->GetPos();
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Selected object at (%.0f, %.0f)", vPos.x, vPos.y);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
}

void CScene_Tool::DeselectObject()
{
    m_pSelectedObject = nullptr;
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Object deselected");
}

void CScene_Tool::DeleteObjectAtPosition(Vec2 _vPos)
{
    // 클릭한 위치에서 오브젝트 찾기
    CObject* pTargetObj = FindObjectAtPosition(_vPos);

    if (!pTargetObj)
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"No object to delete");
        return;
    }

    // 선택된 오브젝트가 삭제 대상이라면 선택 해제
    if (m_pSelectedObject == pTargetObj)
    {
        m_pSelectedObject = nullptr;
    }

    // 벡터에서 직접 제거
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(GetGroupObject((GROUP_TYPE)i));

        auto iter = find(vecObj.begin(), vecObj.end(), pTargetObj);
        if (iter != vecObj.end())
        {
            delete pTargetObj;  // 메모리 해제
            vecObj.erase(iter); // 벡터에서 제거

            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Object deleted successfully! Remaining: %d", (int)vecObj.size());
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
            return;
        }
    }
}

void CScene_Tool::RenderSelectedObject(HDC _dc)
{
    if (!m_pSelectedObject)
        return;

    Vec2 vPos = m_pSelectedObject->GetPos();
    Vec2 vScale = m_pSelectedObject->GetScale();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 선택 표시 (노란색 테두리)
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0)); // 노란색
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 선택 사각형 (약간 더 크게)
    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f - 3),
        (int)(vRenderPos.y - vScale.y / 2.f - 3),
        (int)(vRenderPos.x + vScale.x / 2.f + 3),
        (int)(vRenderPos.y + vScale.y / 2.f + 3));

    // 드래그 중일 때 추가 표시
    if (m_bDragging)
    {
        // 점선으로 이동 경로 표시
        HPEN hDragPen = CreatePen(PS_DOT, 1, RGB(255, 255, 100));
        HPEN hOldDragPen = (HPEN)SelectObject(_dc, hDragPen);

        Vec2 vDragStartRender = CCamera::GetInst()->GetRenderPos(m_vDragStartPos);
        MoveToEx(_dc, (int)vDragStartRender.x, (int)vDragStartRender.y, nullptr);
        LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y);

        SelectObject(_dc, hOldDragPen);
        DeleteObject(hDragPen);
    }

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);
}

void CScene_Tool::SaveLevel(const wstring& _strFileName)
{
    // 레벨 데이터 수집
    tLevelData levelData;
    levelData.strLevelName = _strFileName;
    levelData.iVersion = 1;

    // 플레이어 스폰 위치 찾기
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty())
    {
        levelData.vPlayerSpawn = vecPlayer[0]->GetPos();
    }

    // 모든 오브젝트 데이터 수집 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j] && !vecObj[j]->IsDead()) // 유효하고 살아있는 오브젝트만
            {
                tLevelObjectData objData;
                objData.eGroupType = (GROUP_TYPE)i;
                objData.vPos = vecObj[j]->GetPos();
                objData.vScale = vecObj[j]->GetScale();
                objData.iSubType = 0; // 추후 확장 가능

                // 타일의 경우 타일 타입 정보도 저장
                if (objData.eGroupType == GROUP_TYPE::TILE)
                {
                    CTile* pTile = dynamic_cast<CTile*>(vecObj[j]);
                    if (pTile)
                    {
                        objData.iSubType = (int)pTile->GetTileType();
                    }
                }
                // 몬스터의 경우 몬스터 타입 정보 저장 (추후 확장용)
                else if (objData.eGroupType == GROUP_TYPE::MONSTER)
                {
                    // 현재는 모든 몬스터가 WADDLE_DEE이므로 기본값 사용
                    objData.iSubType = (int)OBJECT_TYPE::MONSTER_WADDLE_DEE;
                }

                levelData.vecObjects.push_back(objData);
            }
        }
    }

    // 파일로 저장
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strLevelDir = strContentPath + L"level\\";
    wstring strFullPath = strLevelDir + _strFileName + L".lvl";

    // 디렉토리가 없으면 생성
    CreateDirectory(strLevelDir.c_str(), nullptr);

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"wb");

    if (pFile)
    {
        // 버전 정보
        fwrite(&levelData.iVersion, sizeof(int), 1, pFile);

        // 레벨 이름 길이 및 이름
        size_t nameLen = levelData.strLevelName.length();
        fwrite(&nameLen, sizeof(size_t), 1, pFile);
        fwrite(levelData.strLevelName.c_str(), sizeof(wchar_t), nameLen, pFile);

        // 플레이어 스폰 위치
        fwrite(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 오브젝트 개수
        size_t objCount = levelData.vecObjects.size();
        fwrite(&objCount, sizeof(size_t), 1, pFile);

        // 각 오브젝트 데이터
        for (const auto& objData : levelData.vecObjects)
        {
            fwrite(&objData, sizeof(tLevelObjectData), 1, pFile);
        }

        fclose(pFile);

        // 성공 메시지
        wchar_t szMsg[256];
        swprintf_s(szMsg, L"Level Saved: %s (%d objects)", _strFileName.c_str(), (int)objCount);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
    }
    else
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Failed to save level!");
    }
}

void CScene_Tool::LoadLevel(const wstring& _strFileName)
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strFullPath = strContentPath + L"level\\" + _strFileName + L".lvl";

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"rb");

    if (!pFile)
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Failed to load level!");
        return;
    }

    // 기존 오브젝트들 삭제 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(GetGroupObject((GROUP_TYPE)i));
        for (CObject* pObj : vecObj)
        {
            if (pObj)
            {
                delete pObj;
            }
        }
        vecObj.clear();
    }

    // 선택 해제
    m_pSelectedObject = nullptr;

    // 파일에서 데이터 읽기
    tLevelData levelData;

    // 버전 확인
    fread(&levelData.iVersion, sizeof(int), 1, pFile);

    if (levelData.iVersion != 1)
    {
        fclose(pFile);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Unsupported level version!");
        return;
    }

    // 레벨 이름
    size_t nameLen;
    fread(&nameLen, sizeof(size_t), 1, pFile);
    wchar_t* szName = new wchar_t[nameLen + 1];
    fread(szName, sizeof(wchar_t), nameLen, pFile);
    szName[nameLen] = L'\0';
    levelData.strLevelName = szName;
    delete[] szName;

    // 플레이어 스폰 위치
    fread(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

    // 플레이어 위치 설정
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty())
    {
        vecPlayer[0]->SetPos(levelData.vPlayerSpawn);
    }

    // 오브젝트 개수
    size_t objCount;
    fread(&objCount, sizeof(size_t), 1, pFile);

    // 각 오브젝트 생성
    for (size_t i = 0; i < objCount; ++i)
    {
        tLevelObjectData objData;
        fread(&objData, sizeof(tLevelObjectData), 1, pFile);

        CObject* pObj = nullptr;

        switch (objData.eGroupType)
        {
        case GROUP_TYPE::MONSTER:
        {
            // 몬스터 타입에 따라 생성 (현재는 WADDLE_DEE만)
            OBJECT_TYPE monsterType = (OBJECT_TYPE)objData.iSubType;
            pObj = CObjectFactory::CreateObject(monsterType, objData.vPos);
        }
        break;

        case GROUP_TYPE::TILE:
        {
            // 타일 타입에 따라 생성
            OBJECT_TYPE tileType = (OBJECT_TYPE)objData.iSubType;
            pObj = CObjectFactory::CreateObject(tileType, objData.vPos);
        }
        break;

        case GROUP_TYPE::ITEM:
        {
            // 아이템 타입에 따라 생성 (추후 확장)
            OBJECT_TYPE itemType = (OBJECT_TYPE)objData.iSubType;
            pObj = CObjectFactory::CreateObject(itemType, objData.vPos);
        }
        break;

        case GROUP_TYPE::SPECIAL:
        {
            // 특수 오브젝트 타입에 따라 생성 (추후 확장)
            OBJECT_TYPE specialType = (OBJECT_TYPE)objData.iSubType;
            pObj = CObjectFactory::CreateObject(specialType, objData.vPos);
        }
        break;

        default:
            continue;
        }

        if (pObj)
        {
            pObj->SetPos(objData.vPos);
            pObj->SetScale(objData.vScale);
            AddObject(pObj, objData.eGroupType);
        }
    }

    fclose(pFile);

    // 성공 메시지
    wchar_t szMsg[256];
    swprintf_s(szMsg, L"Level Loaded: %s (%d objects)", levelData.strLevelName.c_str(), (int)objCount);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
}

void CScene_Tool::QuickSave()
{
    SaveLevel(L"quicksave");
}

void CScene_Tool::QuickLoad()
{
    LoadLevel(L"quicksave");
}