#include "pch.h"
#include "CEditorCore.h"

// 하위 시스템 include
#include "CEditorUI.h"
#include "CEditorInput.h"
#include "CEditorRenderer.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"
#include "CEditorCameraController.h"
#include "CEditorToolbar.h"

#include "CBackground.h"
#include "CObject.h"
#include "CScene.h"
#include "CCore.h"
#include "CTimeMgr.h"
#include "CGrid.h"

CEditorCore::CEditorCore()
    : m_pUI(nullptr)
    , m_pInput(nullptr)
    , m_pRenderer(nullptr)
    , m_pFileManager(nullptr)
    , m_pObjectManager(nullptr)
    , m_pCameraController(nullptr)
    , m_pToolbar(nullptr)
    , m_pWorkingScene(nullptr)
    , m_eCurrentMode(EDITOR_MODE::NONE)
    , m_bShowUI(true)
    , m_vMousePos{}
    , m_bMouseClick(false)
    , m_fClickTime(0.f)
    , m_pSelectedObject(nullptr)
    , m_bDragging(false)
    , m_vDragStartPos{}
{
}

CEditorCore::~CEditorCore()
{
    Shutdown();
}

void CEditorCore::Initialize(CScene* _pScene)
{
    m_pWorkingScene = _pScene;

    // 하위 시스템들 생성
    m_pUI = new CEditorUI();
    m_pInput = new CEditorInput();
    m_pRenderer = new CEditorRenderer();
    m_pFileManager = new CEditorFileManager();
    m_pObjectManager = new CEditorObjectManager();
    m_pCameraController = new CEditorCameraController();
    m_pToolbar = new CEditorToolbar();

    // 하위 시스템들 초기화
    m_pUI->Initialize(this, _pScene);
    m_pInput->Initialize(this);
    m_pRenderer->Initialize(this);
    m_pFileManager->Initialize(this, _pScene);
    m_pObjectManager->Initialize(this);
    m_pCameraController->Initialize(this);
    m_pToolbar->Initialize(this);

    // 그리드 시스템 초기화
    CGrid::GetInst()->init();

    // 기본 설정
    m_eCurrentMode = EDITOR_MODE::NONE;
    m_bShowUI = true;
    m_pSelectedObject = nullptr;
    m_bDragging = false;
}

void CEditorCore::Update()
{
    // 클릭 시간 업데이트
    UpdateClickTime(CTimeMgr::GetInst()->GetfDT());

    // 하위 시스템들 업데이트 (순서 중요!)
    m_pCameraController->Update();      // 카메라 먼저
    m_pInput->Update();                 // 입력 처리
    m_pObjectManager->Update();         // 오브젝트 관리 (배경 업데이트 포함)
    m_pToolbar->Update();               // 툴바 업데이트
}

void CEditorCore::Render(HDC _dc)
{
    // 하위 시스템들 렌더링 (순서 중요!)

    // 1. 배경 렌더링 (ObjectManager에서 처리)
    if (m_pObjectManager->GetCurrentBackground())
    {
        m_pObjectManager->GetCurrentBackground()->Render(_dc);
    }

    // 2. 그리드 렌더링
    CGrid::GetInst()->Render(_dc);

    // 3. **Scene의 모든 오브젝트 렌더링 추가!**
    if (m_pWorkingScene)
    {
        m_pWorkingScene->CScene::Render(_dc);  // CScene::Render() 명시적 호출
    }

    // 4. 에디터 전용 렌더링 (미리보기, 선택 박스 등)
    m_pRenderer->Render(_dc);

    // 5. 툴바 렌더링 (UI보다 먼저)
    m_pToolbar->Render(_dc);

    // 6. UI 렌더링 (맨 앞)
    if (m_bShowUI)
    {
        m_pUI->Render(_dc);
    }
}

void CEditorCore::Shutdown()
{
    // 하위 시스템들 삭제
    if (m_pUI)
    {
        delete m_pUI;
        m_pUI = nullptr;
    }
    if (m_pInput)
    {
        delete m_pInput;
        m_pInput = nullptr;
    }
    if (m_pRenderer)
    {
        delete m_pRenderer;
        m_pRenderer = nullptr;
    }
    if (m_pFileManager)
    {
        delete m_pFileManager;
        m_pFileManager = nullptr;
    }
    if (m_pObjectManager)
    {
        delete m_pObjectManager;
        m_pObjectManager = nullptr;
    }
    if (m_pCameraController)
    {
        delete m_pCameraController;
        m_pCameraController = nullptr;
    }
    if (m_pToolbar)
    {
        delete m_pToolbar;
        m_pToolbar = nullptr;
    }

    // 선택 해제
    m_pSelectedObject = nullptr;
    m_pWorkingScene = nullptr;
}

void CEditorCore::ChangeMode(EDITOR_MODE _eMode)
{
    m_eCurrentMode = _eMode;

    // 모드 변경 시 선택 해제
    if (_eMode != EDITOR_MODE::SELECT)
    {
        DeselectObject();
    }

    // 모드 변경 시 윈도우 타이틀 업데이트
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Level Editor - Mode: %s", GetModeString());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

const wchar_t* CEditorCore::GetModeString()
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::NONE:         return L"Normal";
    case EDITOR_MODE::PLACE_MONSTER: return L"Place Monster";
    case EDITOR_MODE::PLACE_ITEM:   return L"Place Item";
    case EDITOR_MODE::PLACE_TILE:   return L"Place Tile";
    case EDITOR_MODE::PLACE_SPECIAL: return L"Place Special";
    case EDITOR_MODE::SELECT:       return L"Select";
    case EDITOR_MODE::ERASE:        return L"Erase";
    case EDITOR_MODE::CAMERA_MOVE:  return L"Camera Move";
    case EDITOR_MODE::BACKGROUND:   return L"Background";
    case EDITOR_MODE::PLAYER_SPAWN: return L"Player Spawn";
    default:                        return L"Unknown";
    }
}

void CEditorCore::SetMouseClick(bool _bClick, float _fTime)
{
    m_bMouseClick = _bClick;
    if (_bClick)
    {
        m_fClickTime = _fTime;
    }
}

void CEditorCore::UpdateClickTime(float _fDT)
{
    if (m_fClickTime > 0.f)
    {
        m_fClickTime -= _fDT;
        if (m_fClickTime <= 0.f)
        {
            m_bMouseClick = false;
        }
    }
}

void CEditorCore::SetSelectedObject(CObject* _pObj)
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

void CEditorCore::DeselectObject()
{
    m_pSelectedObject = nullptr;
    m_bDragging = false;
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Object deselected");
}