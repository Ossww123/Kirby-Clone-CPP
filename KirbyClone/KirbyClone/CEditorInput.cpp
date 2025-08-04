#include "pch.h"
#include "CEditorInput.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"
#include "CEditorFileManager.h"
#include "CEditorCameraController.h"
#include "CEditorToolbar.h"
#include "CEditorUI.h"

#include "CObject.h"
#include "CEventMgr.h"
#include "CKeyMgr.h"
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

        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Level Editor - UI %s", bShowUI ? L"OFF" : L"ON");
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }

    // HOME 키: 카메라를 원점(0, 0)으로 이동
    if (KEY_TAP(KEY::HOME))
    {
        m_pEditorCore->GetCameraController()->ResetCameraPosition();
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Camera reset to origin (0, 0)");
    }

    // Ctrl + B: 레벨 경계 설정 모드
    if (KEY_TAP(KEY::B) && KEY_HOLD(KEY::CTRL))
    {
        // 현재 오브젝트들의 범위를 계산해서 적절한 경계 자동 설정
        m_pEditorCore->GetCameraController()->AutoSetBoundsFromObjects();
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level bounds auto-calculated from objects");
    }

    // ALT + 다른 키 조합들 (ALT 키를 modifier로 사용)
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
        // ALT + I: 오브젝트 개수 정보 표시
        else if (KEY_TAP(KEY::I))
        {
            int objectCount = m_pEditorCore->GetObjectManager()->GetTotalObjectCount();
            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Total Objects: %d", objectCount);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
        }
        // ALT + P: 플레이어 스폰 표시 토글
        else if (KEY_TAP(KEY::P))
        {
            bool bShow = m_pEditorCore->GetObjectManager()->IsShowPlayerSpawn();
            m_pEditorCore->GetObjectManager()->SetShowPlayerSpawn(!bShow);

            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Player Spawn Display: %s", bShow ? L"OFF" : L"ON");
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
        }
    }
}

void CEditorInput::UpdateGridInput()
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
    if (KEY_HOLD(KEY::CTRL) && KEY_TAP(KEY::G))
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

void CEditorInput::UpdateModeInput()
{
    // ESC - 기본 모드로
    if (KEY_TAP(KEY::ESC))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::NONE);
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
    else if (KEY_TAP(KEY::R))
    {
        m_pEditorCore->ChangeMode(EDITOR_MODE::PLAYER_SPAWN);
    }
}

void CEditorInput::UpdateMouseInput()
{
    // 1. 원본 스크린 좌표 얻기 (그리드 스냅 적용 전)
    Vec2 vRawMousePos = CKeyMgr::GetInst()->GetMousePos();  // 스크린 좌표

    // 2. 그리드 스냅이 적용된 월드 좌표 계산 및 저장
    UpdateMousePosition();
    Vec2 vGridSnappedPos = m_pEditorCore->GetMousePos();

    // 마우스 클릭 처리
    if (KEY_TAP(KEY::MOUSE_LEFT))
    {
        // 1순위: 툴바 이벤트 처리 (원본 스크린 좌표 사용)
        if (m_pEditorCore->GetToolbar()->HandleMouseClick(vRawMousePos))
        {
            return; // 툴바에서 처리했으면 더 이상 진행하지 않음
        }

        // 2순위: 오브젝트 팔레트 클릭 처리 (원본 스크린 좌표 사용)
        if (m_pEditorCore->GetUI()->HandlePaletteClick(vRawMousePos))
        {
            return; // 팔레트에서 처리했으면 더 이상 진행하지 않음
        }

        // 3순위: 기본 마우스 클릭 처리 (그리드 스냅된 좌표 사용)
        m_pEditorCore->SetMouseClick(true, 0.5f);
        HandleMouseClick();
    }

    // 마우스 버튼을 떼면 드래그 종료 + 툴바 MouseUp 이벤트 처리
    if (KEY_AWAY(KEY::MOUSE_LEFT))
    {
        // 툴바 MouseUp 이벤트도 원본 스크린 좌표 사용
        m_pEditorCore->GetToolbar()->HandleMouseUp(vRawMousePos);

        // 드래그 종료
        m_pEditorCore->SetDragging(false);
    }

    // 마우스 이동 이벤트도 툴바에는 원본 스크린 좌표 전달
    m_pEditorCore->GetToolbar()->HandleMouseMove(vRawMousePos);

    // 마우스 휠 스크롤 처리 (팔레트 스크롤용)
    if (m_pEditorCore->GetUI()->IsInPaletteArea(vRawMousePos))
    {
        // 마우스 휠 입력 처리 (Windows API 사용 시)
        // 실제 구현은 프로젝트의 입력 시스템에 따라 달라질 수 있음
        // 예: if (KEY_TAP(KEY::MOUSE_WHEEL_UP)) { ... }
    }

    // 드래그 중이면 선택된 오브젝트 이동 (그리드 스냅된 좌표 사용)
    if (m_pEditorCore->IsDragging() &&
        m_pEditorCore->GetSelectedObject() &&
        m_pEditorCore->GetCurrentMode() == EDITOR_MODE::SELECT &&
        !m_pEditorCore->GetToolbar()->IsInToolbarArea(vRawMousePos) &&
        !m_pEditorCore->GetUI()->IsInPaletteArea(vRawMousePos))  // 팔레트 영역도 제외
    {
        m_pEditorCore->GetSelectedObject()->SetPos(vGridSnappedPos);  // 오브젝트 이동은 스냅된 좌표
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

    case EDITOR_MODE::NONE:
    case EDITOR_MODE::CAMERA_MOVE:
    default:
        // 기본 모드에서는 클릭 위치만 표시
    {
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Clicked at: (%.0f, %.0f)", vMousePos.x, vMousePos.y);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
    break;
    }
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