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
    UpdateGeneralInput();       // UI 토글 등 최우선
    UpdateFileInput();          // 파일 작업 (Ctrl 조합)
    UpdateGridInput();          // 그리드 설정
    UpdateModeInput();          // 모드 전환
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

    // HOME 키: 카메라를 원점으로 이동
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
        // ALT + R: 기본 설정으로 리셋
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
    // 파일 관련 입력 (Ctrl 조합키들)
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
        else if (KEY_TAP(KEY::T))
        {
            // 게임 모드로 전환 (기존 CScene_Tool 코드에서 가져옴)
            tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::START);
            CEventMgr::GetInst()->AddEvent(event);
        }
    }

    // 빠른 저장/로드 (Ctrl 없이)
    if (KEY_TAP(KEY::F))
    {
        m_pEditorCore->GetFileManager()->QuickSave();
    }
    else if (KEY_TAP(KEY::L))
    {
        m_pEditorCore->GetFileManager()->QuickLoad();
    }
}


void CEditorInput::UpdateGridInput()
{
    // 그리드 표시 토글 (G 키)
    if (KEY_TAP(KEY::G))
    {
        bool bShowGrid = CGrid::GetInst()->IsShowGrid();
        CGrid::GetInst()->SetShowGrid(!bShowGrid);
    }

    // 그리드 스냅 토글 (Ctrl + G)
    if (KEY_HOLD(KEY::CTRL) && KEY_TAP(KEY::G))
    {
        bool bSnapToGrid = CGrid::GetInst()->IsSnapToGrid();
        CGrid::GetInst()->SetSnapToGrid(!bSnapToGrid);
    }

    // 그리드 크기 조절 (1, 2, 3, 4 키)
    if (KEY_TAP(KEY::ALPHA_1))
    {
        CGrid::GetInst()->SetGridSizePreset(1);  // 32px
    }
    else if (KEY_TAP(KEY::ALPHA_2))
    {
        CGrid::GetInst()->SetGridSizePreset(2);  // 64px
    }
    else if (KEY_TAP(KEY::ALPHA_3))
    {
        CGrid::GetInst()->SetGridSizePreset(3);  // 128px
    }
    else if (KEY_TAP(KEY::ALPHA_4))
    {
        CGrid::GetInst()->SetGridSizePreset(4);  // 256px
    }
}

void CEditorInput::UpdateModeInput()
{
    // ESC - 기본 모드로
    if (KEY_TAP(KEY::ESC))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::NORMAL);
        return;
    }

    // 오브젝트 배치 모드들
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

    // 편집 도구 모드들
    else if (KEY_TAP(KEY::S))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::SELECT);
    }
    else if (KEY_TAP(KEY::E))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::ERASE);
    }

    // 특수 모드들
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

void CEditorInput::UpdateMouseInput()
{
    // 원본 스크린 좌표 얻기 (그리드 스냅 적용 전)
    Vec2 vRawMousePos = CKeyMgr::GetInst()->GetMousePos();

    // 그리드 스냅이 적용된 월드 좌표 계산 및 저장
    UpdateMousePosition();

    // 마우스 클릭 처리
    if (KEY_TAP(KEY::MOUSE_LEFT))
    {
        // 툴바 이벤트 처리 우선 (원본 스크린 좌표 사용)
        if (m_pEditorCore->GetToolbar() &&
            m_pEditorCore->GetToolbar()->HandleMouseClick(vRawMousePos))
        {
            return; // 툴바에서 처리했으면 더 이상 진행하지 않음
        }

        // UI 팔레트 클릭 처리 (원본 스크린 좌표 사용)
        if (m_pEditorCore->GetUI() &&
            m_pEditorCore->GetUI()->HandlePaletteClick(vRawMousePos))
        {
            return; // 팔레트에서 처리했으면 더 이상 진행하지 않음
        }

        // 기본 마우스 클릭 처리 (그리드 스냅된 좌표 사용)
        HandleMouseClick();
    }

    // 마우스 버튼을 떼면 드래그 종료
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

    // 마우스 이동 이벤트 전달
    if (m_pEditorCore->GetToolbar())
    {
        m_pEditorCore->GetToolbar()->HandleMouseMove(vRawMousePos);
    }

    // 드래그 중이면 선택된 오브젝트 이동
    if (m_pEditorCore->IsDragging() &&
        m_pEditorCore->GetSelectedObject() &&
        m_pEditorCore->GetCurrentMode() == EDITOR_MODE::SELECT)
    {
        // 툴바나 UI 영역이 아닐 때만 오브젝트 이동
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
    // Tab키: 카테고리 내 다음 오브젝트
    if (KEY_TAP(KEY::TAB) && !KEY_HOLD(KEY::SHIFT))
    {
        m_pEditorCore->GetObjectManager()->NextObjectInCategory();
    }
    // Shift+Tab키: 카테고리 내 이전 오브젝트
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
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPosition(vMousePos);
        break;

    case EDITOR_MODE::SELECT:
    {
        // 클릭한 위치에서 오브젝트 찾기
        CObject* pClickedObj = m_pEditorCore->GetObjectManager()->FindObjectAtPosition(vMousePos);
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
        m_pEditorCore->GetObjectManager()->DeleteObjectAtPosition(vMousePos);
        break;

    case EDITOR_MODE::BACKGROUND:
    {
        // 배경 모드에서는 클릭으로 다음 배경으로 변경
        m_pEditorCore->GetObjectManager()->NextBackground();
    }
    break;
    case EDITOR_MODE::PLACE_STAGE:
        // Stage Image 모드에서는 클릭으로 스테이지 이미지 변경
        HandleStageImageClick();
        break;
    case EDITOR_MODE::NORMAL:
    case EDITOR_MODE::CAMERA_MOVE:
    default:
        // 기본 모드에서는 클릭 위치만 표시
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
    // 배경 모드에서 Q/E로 배경 변경
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
    // 타일 모드에서 Q/E로 타일 시각 타입 변경
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
    // Q/E 키로 스테이지 이미지 변경
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

    // R 키로 스테이지 이미지를 좌하단으로 재배치
    if (KEY_TAP(KEY::R))
    {
        CStageImage* pCurrent = CStageMgr::GetInst()->GetCurrentStageImage();
        if (pCurrent)
        {
            pCurrent->SetImageToBottomLeft();
        }
    }
}

void CEditorInput::HandleStageImageClick()
{
    // 현재 스테이지 타입 가져오기
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();

    // 사용 가능한 스테이지 타입들 가져오기
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

    // 다음 스테이지 타입으로 변경
    currentIndex = (currentIndex + 1) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE nextType = availableTypes[currentIndex];

    // 스테이지 이미지 변경
    CStageMgr::GetInst()->SetCurrentStageImage(nextType);
}


void CEditorInput::ResetStageImageToBottomLeft()
{
    CStageImage* pCurrentStage = CStageMgr::GetInst()->GetCurrentStageImage();
    if (pCurrentStage)
    {
        pCurrentStage->SetImageToBottomLeft();
    }
}

// 이전 스테이지 이미지로 변경
void CEditorInput::PrevStageImage()
{
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.size() <= 1)
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

    // 이전 스테이지 타입으로 변경
    currentIndex = (currentIndex - 1 + (int)availableTypes.size()) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE prevType = availableTypes[currentIndex];

    CStageMgr::GetInst()->SetCurrentStageImage(prevType);
}

// 다음 스테이지 이미지로 변경
void CEditorInput::NextStageImage()
{
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst()->GetCurrentStageType();
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst()->GetAvailableStageImageTypes();

    if (availableTypes.size() <= 1)
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

    // 다음 스테이지 타입으로 변경
    currentIndex = (currentIndex + 1) % (int)availableTypes.size();
    STAGE_IMAGE_TYPE nextType = availableTypes[currentIndex];

    CStageMgr::GetInst()->SetCurrentStageImage(nextType);
}

// 커스텀 스테이지 이미지 로드
void CEditorInput::LoadCustomStageImage()
{
    // 파일 다이얼로그 열기
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
        // 상대 경로로 변환 (content 폴더 기준)
        wstring strFullPath = szFile;
        wstring strContentPath = CPathMgr::GetInst()->GetContentPath();

        wstring strRelativePath;
        if (strFullPath.find(strContentPath) == 0)
        {
            // content 폴더 내부의 파일인 경우
            strRelativePath = strFullPath.substr(strContentPath.length());
        }
        else
        {
            // 외부 파일인 경우 파일명만 사용
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

        // 커스텀 스테이지 이미지 생성
        CStageMgr::GetInst()->CreateStageImage(STAGE_IMAGE_TYPE::CUSTOM, strRelativePath);
        CStageMgr::GetInst()->SetCurrentStageImage(STAGE_IMAGE_TYPE::CUSTOM);
    }
}