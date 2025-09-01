#include "pch.h"
#include "CEditorInput.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"
#include "CEditorFileManager.h"
#include "CEditorCameraController.h"
#include "CEditorToolbar.h"
#include "CEditorUI.h"

#include "CStageImage.h"
#include "CObject.h"
#include "CDoor.h"
#include "CEventMgr.h"
#include "CKeyMgr.h"
#include "CStageMgr.h"
#include "CPathMgr.h"
#include "CGrid.h"
#include "CCamera.h"
#include "CCore.h"

CEditorInput::CEditorInput()
    : m_pEditorCore(nullptr)
{
}

CEditorInput::~CEditorInput()
{
}

void CEditorInput::Initialize(CEditorCore* _pCore)
{
    m_pEditorCore = _pCore;
}

void CEditorInput::Update()
{
    // 입력 처리 순서 (우선순위 순)
    UpdateGeneralInput();       // UI 토글 및 주요키
    UpdateFileInput();          // 파일 작업 (Ctrl 단축키)
    UpdateGridInput();          // 그리드 제어
    UpdateModeInput();          // 모드 변환
    UpdateSelectedObjectInput(); // 선택된 오브젝트 제어
    UpdateMouseInput();         // 마우스 처리
    UpdateObjectSelection();    // 오브젝트 선택 (Tab 키)
    HandleModeSpecificInput();  // 현재 모드에 특화된 입력
}

void CEditorInput::UpdateGeneralInput()
{
    // UI 토글 (H 키)
    if (KEY_TAP(KEY::H))
    {
        bool bShowUI = m_pEditorCore->IsShowUI();
        m_pEditorCore->SetShowUI(!bShowUI);
    }

    // HOME 키: 카메라 원점으로 이동
    if (KEY_TAP(KEY::HOME))
    {
        m_pEditorCore->GetCameraController()->ResetCameraPosition();
    }

    // ALT + 다른 키 조합들
    if (KEY_HOLD(KEY::ALT))
    {
        // ALT + C: 모든 오브젝트 삭제
        if (KEY_TAP(KEY::C))
        {
            m_pEditorCore->GetObjectManager()->ClearAllObjects();
        }
        // ALT + R: 기본 오브젝트 추가
        else if (KEY_TAP(KEY::R))
        {
            m_pEditorCore->GetObjectManager()->ResetToDefault();
        }
        // ALT + P: 플레이어 스폰 표시 토글
        else if (KEY_TAP(KEY::P))
        {
            bool bShow = m_pEditorCore->GetObjectManager()->IsShowPlayerSpawn();
            m_pEditorCore->GetObjectManager()->SetShowPlayerSpawn(!bShow);
        }
    }
}

void CEditorInput::UpdateFileInput()
{
    // 파일 관련 입력 (Ctrl 단축키들)
    if (KEY_HOLD(KEY::CTRL))
    {
        if (KEY_TAP(KEY::S))
        {
            m_pEditorCore->GetFileManager()->SaveAsDialog();
        }
        else if (KEY_TAP(KEY::O))
        {
            m_pEditorCore->GetFileManager()->OpenDialog();
        }
    }
}

void CEditorInput::UpdateGridInput()
{
    // 그리드 표시 토글 (G 키)
    if (KEY_HOLD(KEY::CTRL) && KEY_TAP(KEY::G))
    {
        bool bShowGrid = CGrid::GetInst()->IsShowGrid();
        CGrid::GetInst()->SetShowGrid(!bShowGrid);
    }

    // 그리드 스냅 토글 (Alt + G)
    if (KEY_HOLD(KEY::ALT) && KEY_TAP(KEY::G))
    {
        bool bSnapToGrid = CGrid::GetInst()->IsSnapToGrid();
        CGrid::GetInst()->SetSnapToGrid(!bSnapToGrid);
    }

    // 그리드 크기 변경 (1, 2, 3, 4 키)
    if (KEY_TAP(KEY::ALPHA_1))
    {
        CGrid::GetInst()->SetGridSizePreset(1);  // 16px
    }
    else if (KEY_TAP(KEY::ALPHA_2))
    {
        CGrid::GetInst()->SetGridSizePreset(2);  // 32px
    }
    else if (KEY_TAP(KEY::ALPHA_3))
    {
        CGrid::GetInst()->SetGridSizePreset(3);  // 64px
    }
    else if (KEY_TAP(KEY::ALPHA_4))
    {
        CGrid::GetInst()->SetGridSizePreset(4);  // 128px
    }
}

void CEditorInput::UpdateModeInput()
{
    // ESC - 기본 모드
    if (KEY_TAP(KEY::ESC))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::NORMAL);
        return;
    }

    // 오브젝트 배치 모드
    if (KEY_TAP(KEY::M))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_MONSTER);
        m_pEditorCore->GetObjectManager()->ChangeObjectCategory(L"Monster");
    }
    else if (KEY_TAP(KEY::I))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_ITEM);
        m_pEditorCore->GetObjectManager()->ChangeObjectCategory(L"Item");
    }
    else if (KEY_TAP(KEY::T))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_TILE);
        m_pEditorCore->GetObjectManager()->ChangeObjectCategory(L"Tile");
    }
    else if (KEY_TAP(KEY::P))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_SPECIAL);
        m_pEditorCore->GetObjectManager()->ChangeObjectCategory(L"Special");
    }

    // 조작 모드 전환
    else if (KEY_TAP(KEY::S))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::SELECT);
    }
    else if (KEY_TAP(KEY::E))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::ERASE);
    }

    // 특수 모드
    else if (KEY_TAP(KEY::B))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::BACKGROUND);
    }
    else if (KEY_TAP(KEY::ALPHA_9))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLACE_STAGE);
    }
    else if (KEY_TAP(KEY::R))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLAYER_SPAWN);
    }
}

void CEditorInput::UpdateSelectedObjectInput()
{
    // 선택된 오브젝트의 속성 입력 처리
    CObject* pSelected = m_pEditorCore->GetSelectedObject();
    if (!pSelected)
        return;

    // 문 오브젝트의 속성 설정 처리
    if (pSelected->GetType() == OBJECT_TYPE::OBJECT_DOOR)
    {
        CDoor* pDoor = dynamic_cast<CDoor*>(pSelected);
        if (pDoor)
        {
            HandleDoorPropertyInput(pDoor);
        }
    }
    // 다른 특수 오브젝트들도 추가 확장 가능
}

void CEditorInput::UpdateMouseInput()
{
    // 원시 마우스 좌표 가져오기 (그리드 스냅 적용 전)
    Vec2 vRawMousePos = CKeyMgr::GetInst()->GetMousePos();

    // 그리드 스냅이 적용된 마우스 좌표 계산 및 저장
    UpdateMousePosition();

    // 마우스 클릭 처리
    if (KEY_TAP(KEY::MOUSE_LEFT))
    {
        // 툴바 이벤트 처리 우선 (원시 마우스 좌표 사용)
        if (m_pEditorCore->GetToolbar() &&
            m_pEditorCore->GetToolbar()->HandleMouseClick(vRawMousePos))
        {
            return; // 툴바에서 처리했으므로 더 이상 진행하지 않음
        }

        // UI 팔레트 클릭 처리 (원시 마우스 좌표 사용)
        if (m_pEditorCore->GetUI() &&
            m_pEditorCore->GetUI()->HandlePaletteClick(vRawMousePos))
        {
            return; // 팔레트에서 처리했으므로 더 이상 진행하지 않음
        }

        // UI 속성 패널 클릭 처리 (스크린 좌표 사용)
        if (m_pEditorCore->GetUI() &&
            m_pEditorCore->GetUI()->HandlePropertyPanelClick(vRawMousePos))
        {
            return; // 속성 패널에서 처리했으므로 더 이상 진행하지 않음
        }

        // 기본 마우스 클릭 처리 (그리드 스냅된 좌표 사용)
        HandleMouseClick();
    }

    // 마우스 버튼을 뗀 드래그 종료
    if (KEY_AWAY(KEY::MOUSE_LEFT))
    {
        // 툴바 MouseUp 이벤트 처리
        if (m_pEditorCore->GetToolbar())
        {
            m_pEditorCore->GetToolbar()->HandleMouseUp(vRawMousePos);
        }

        // 드래그 종료
        if (m_pEditorCore->IsDragging())
        {
            m_pEditorCore->SetDragging(false);
        }
    }

    // 마우스 이동 이벤트 처리
    if (m_pEditorCore->GetToolbar())
    {
        m_pEditorCore->GetToolbar()->HandleMouseMove(vRawMousePos);
    }

    // 드래그 중이면 선택된 오브젝트 이동
    if (m_pEditorCore->IsDragging() &&
        m_pEditorCore->GetSelectedObject() &&
        m_pEditorCore->GetCurrentMode() == EDITOR_MODE::SELECT)
    {
        // 툴바나 UI 영역이 아닌 경우 오브젝트 이동
        bool bInToolbar = m_pEditorCore->GetToolbar() &&
            m_pEditorCore->GetToolbar()->IsInToolbarArea(vRawMousePos);
        bool bInPalette = m_pEditorCore->GetUI() &&
            m_pEditorCore->GetUI()->IsInPaletteArea(vRawMousePos);

        if (!bInToolbar && !bInPalette)
        {
            Vec2 vGridSnappedPos = m_pEditorCore->GetMousePos();
            m_pEditorCore->GetSelectedObject()->SetPos(vGridSnappedPos);
        }
    }
}

void CEditorInput::UpdateObjectSelection()
{
    // Tab키: 카테고리내 다음 오브젝트
    if (KEY_TAP(KEY::TAB) && !KEY_HOLD(KEY::SHIFT))
    {
        m_pEditorCore->GetObjectManager()->NextObjectInCategory();
    }
    // Shift+Tab키: 카테고리내 이전 오브젝트
    else if (KEY_TAP(KEY::TAB) && KEY_HOLD(KEY::SHIFT))
    {
        m_pEditorCore->GetObjectManager()->PrevObjectInCategory();
    }
}

void CEditorInput::HandleMouseClick()
{
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode();
    Vec2 vMousePos = m_pEditorCore->GetMousePos();

    switch (eMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
    case EDITOR_MODE::PLACE_ITEM:
    case EDITOR_MODE::PLACE_TILE:
    case EDITOR_MODE::PLACE_SPECIAL:
        m_pEditorCore->GetObjectManager()->PlaceObject(vMousePos);
        break;

    case EDITOR_MODE::PLAYER_SPAWN:
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(vMousePos);
        break;

    case EDITOR_MODE::SELECT:
    {
        // 클릭한 위치에서 오브젝트 찾기
        CObject* pClickedObj = m_pEditorCore->GetObjectManager()->FindObjectAtPos(vMousePos);
        if (pClickedObj)
        {
            m_pEditorCore->SetSelectedObject(pClickedObj);
            m_pEditorCore->SetDragging(true);
            m_pEditorCore->SetDragStartPos(vMousePos);
        }
        else
        {
            // 빈 공간 클릭 시 선택 해제
            m_pEditorCore->DeselectObject();
        }
    }
    break;

    case EDITOR_MODE::ERASE:
        m_pEditorCore->GetObjectManager()->DeleteObjectAtPos(vMousePos);
        break;

    case EDITOR_MODE::BACKGROUND:
    {
        // 배경 모드에서 클릭하면 다음 배경으로 전환
        m_pEditorCore->GetObjectManager()->NextBackground();
    }
    break;
    case EDITOR_MODE::PLACE_STAGE:
        // Stage Image 모드에서 클릭하면 스테이지 이미지 설정
        HandleStageImageClick();
        break;
    case EDITOR_MODE::NORMAL:
    default:
        // 기본 모드에서 클릭 위치의 표시
    break;
    }
}

void CEditorInput::UpdateMousePosition()
{
    // CKeyMgr에서 마우스 월드 좌표 가져오기
    Vec2 vMousePos = CKeyMgr::GetInst()->GetMouseWorldPos();

    // 그리드 스냅 적용
    if (CGrid::GetInst()->IsSnapToGrid())
    {
        vMousePos = CGrid::GetInst()->SnapToGrid(vMousePos);
    }

    m_pEditorCore->SetMousePos(vMousePos);
}

void CEditorInput::HandleModeSpecificInput()
{
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode();

    switch (eMode)
    {
    case EDITOR_MODE::BACKGROUND:
        HandleBackgroundModeInput();
        break;
    case EDITOR_MODE::PLACE_TILE:
        HandleTileModeInput();
        break;
    case EDITOR_MODE::PLACE_STAGE:
        HandleStageImageModeInput();
        break;
    }
}

void CEditorInput::HandleBackgroundModeInput()
{
    // 배경 모드에서 Q/E로 배경 전환
    if (KEY_TAP(KEY::Q))
    {
        m_pEditorCore->GetObjectManager()->PrevBackground();
    }
    else if (KEY_TAP(KEY::E))
    {
        m_pEditorCore->GetObjectManager()->NextBackground();
    }
}

void CEditorInput::HandleTileModeInput()
{
    // 타일 모드에서 Q/E로 타일 비주얼 전환
    if (KEY_TAP(KEY::Q))
    {
        m_pEditorCore->GetObjectManager()->PrevTileVisual();
    }
    else if (KEY_TAP(KEY::E))
    {
        m_pEditorCore->GetObjectManager()->NextTileVisual();
    }
}

void CEditorInput::HandleStageImageModeInput()
{
    // Q/E 키로 스테이지 이미지 전환
    if (KEY_TAP(KEY::Q))
    {
        PrevStageImage();
    }
    else if (KEY_TAP(KEY::E))
    {
        NextStageImage();
    }

    // C 키로 커스텀 스테이지 이미지 로드
    if (KEY_TAP(KEY::C))
    {
        LoadCustomStageImage();
    }

    // R 키로 스테이지 이미지를 바닥왼쪽으로 배치
    if (KEY_TAP(KEY::R))
    {
        CStageImage* pCurrent = CStageMgr::GetInst()->GetCurrentStageImage();
        if (pCurrent && m_pEditorCore && m_pEditorCore->GetCameraController())
        {
            // 현재 맵 크기를 가져와서 사용
            Vec2 vMapSize = m_pEditorCore->GetCameraController()->GetCameraBoundsMax() - 
                           m_pEditorCore->GetCameraController()->GetCameraBoundsMin();
            pCurrent->SetImageToBottomLeft(vMapSize);
        }
    }
}

void CEditorInput::HandleDoorPropertyInput(CDoor* _pDoor)
{
    // 목표 씬 설정 (1~2 키)
    if (KEY_TAP(KEY::ALPHA_1))
    {
        _pDoor->SetTargetScene(SCENE_TYPE::STAGE_01);
        ShowDoorPropertyChanged(_pDoor, L"목표 씬을 '스테이지 1'로 변경");
    }
    else if (KEY_TAP(KEY::ALPHA_2))
    {
        _pDoor->SetTargetScene(SCENE_TYPE::STAGE_02);
        ShowDoorPropertyChanged(_pDoor, L"목표 씬을 '스테이지 2'로 변경");
    }

    // 목표 위치 설정 (Q/W/A/S 키)
    Vec2 currentPos = _pDoor->GetTargetPosition();
    bool posChanged = false;
    float moveStep = 32.f; // 그리드 크기에 맞춤

    if (KEY_TAP(KEY::Q))
    {
        currentPos.x -= moveStep;
        posChanged = true;
    }
    else if (KEY_TAP(KEY::W))
    {
        currentPos.x += moveStep;
        posChanged = true;
    }
    else if (KEY_TAP(KEY::A))
    {
        currentPos.y -= moveStep;
        posChanged = true;
    }
    else if (KEY_TAP(KEY::S))
    {
        currentPos.y += moveStep;
        posChanged = true;
    }

    // 위치가 변경되었는지 확인
    if (posChanged)
    {
        _pDoor->SetTargetPosition(currentPos);

        wchar_t szMessage[128];
        swprintf_s(szMessage, L"목표 위치를 (%.0f, %.0f)로 변경", currentPos.x, currentPos.y);
        ShowDoorPropertyChanged(_pDoor, szMessage);
    }

    // 기본 위치로 리셋 (R 키)
    if (KEY_TAP(KEY::R))
    {
        _pDoor->SetTargetPosition(Vec2(256.f, 384.f)); // 기본 위치
        ShowDoorPropertyChanged(_pDoor, L"목표 위치를 기본값으로 리셋");
    }
}

void CEditorInput::ShowDoorPropertyChanged(CDoor* _pDoor, const wchar_t* _message)
{
    // 윈도우 타이틀바에 상태 메시지 표시 (임시 피드백)
    wchar_t szTitle[256];
    swprintf_s(szTitle, L"Door Editor - %s", _message);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szTitle);

    // 또는 콘솔 로그로 상세 정보 표시
    wprintf(L"[Door Property] %s\n", _message);
}

void CEditorInput::HandleStageImageClick()
{
    // 현재 스테이지 타입 가져오기
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();

    // 모든 사용 가능한 스테이지 타입들 가져오기
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.empty())
        return;

    // 현재 타입의 인덱스 찾기
    int currentIndex = 0;
    for (size_t i = 0; i < availableTypes.size(); ++i)
    {
        if (availableTypes[i] == currentType)
        {
            currentIndex = (int)i;
            break;
        }
    }

    // 다음 스테이지 타입으로 전환
    currentIndex = (currentIndex + 1) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE nextType = availableTypes[currentIndex];

    // 스테이지 이미지 설정
    CStageMgr::GetInst()->SetCurrentStageImage(nextType);
}

void CEditorInput::ResetStageImageToBottomLeft()
{
    CStageImage* pCurrentStage = CStageMgr::GetInst()->GetCurrentStageImage();
    if (pCurrentStage && m_pEditorCore && m_pEditorCore->GetCameraController())
    {
        // 현재 맵 크기를 가져와서 사용
        Vec2 vMapSize = m_pEditorCore->GetCameraController()->GetCameraBoundsMax() - 
                       m_pEditorCore->GetCameraController()->GetCameraBoundsMin();
        pCurrentStage->SetImageToBottomLeft(vMapSize);
    }
}

void CEditorInput::PrevStageImage()
{
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.size() <= 1)
        return;

    // ���� Ÿ���� �ε��� ã��
    int currentIndex = 0;
    for (size_t i = 0; i < availableTypes.size(); ++i)
    {
        if (availableTypes[i] == currentType)
        {
            currentIndex = (int)i;
            break;
        }
    }

    // ���� �������� Ÿ������ ����
    currentIndex = (currentIndex - 1 + (int)availableTypes.size()) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE prevType = availableTypes[currentIndex];

    CStageMgr::GetInst()->SetCurrentStageImage(prevType);
}

void CEditorInput::NextStageImage()
{
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.size() <= 1)
        return;

    // ���� Ÿ���� �ε��� ã��
    int currentIndex = 0;
    for (size_t i = 0; i < availableTypes.size(); ++i)
    {
        if (availableTypes[i] == currentType)
        {
            currentIndex = (int)i;
            break;
        }
    }

    // ���� �������� Ÿ������ ����
    currentIndex = (currentIndex + 1) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE nextType = availableTypes[currentIndex];

    CStageMgr::GetInst()->SetCurrentStageImage(nextType);
}

void CEditorInput::LoadCustomStageImage()
{
    // ���� ���̾�α� ����
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = CCore::GetInst()->GetMainHwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        // ��� ��η� ��ȯ (content ���� ����)
        wstring strFullPath = szFile;
        wstring strContentPath = CPathMgr::GetInst()->GetContentPath();

        wstring strRelativePath;
        if (strFullPath.find(strContentPath) == 0)
        {
            // content ���� ������ ������ ���
            strRelativePath = strFullPath.substr(strContentPath.length());
        }
        else
        {
            // �ܺ� ������ ��� ���ϸ��� ���
            size_t pos = strFullPath.find_last_of(L"\\");
            if (pos != wstring::npos)
            {
                strRelativePath = L"stage\\" + strFullPath.substr(pos + 1);
            }
            else
            {
                strRelativePath = L"stage\\" + strFullPath;
            }
        }

        // Ŀ���� �������� �̹��� ����
        CStageMgr::GetInst()->CreateStageImage(STAGE_IMAGE_TYPE::CUSTOM, strRelativePath);
        CStageMgr::GetInst()->SetCurrentStageImage(STAGE_IMAGE_TYPE::CUSTOM);
    }
}