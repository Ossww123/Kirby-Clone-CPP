#include "pch.h"
#include "CEditorCore.h"
#include "CBackground.h"
#include "CObject.h"
#include "CScene.h"
#include "CCore.h"
#include "CTimeMgr.h"
#include "CGrid.h"
#include "CCamera.h"
#include "CStageMgr.h"

// 하위 시스템
#include "CEditorUI.h"
#include "CEditorInput.h"
#include "CEditorRenderer.h"
#include "CEditorFileManager.h"
#include "CEditorObjectManager.h"
#include "CEditorCameraController.h"
#include "CEditorToolbar.h"

CEditorCore::CEditorCore()
    : m_pUI(nullptr)
    , m_pInput(nullptr)
    , m_pRenderer(nullptr)
    , m_pFileManager(nullptr)
    , m_pObjectManager(nullptr)
    , m_pCameraController(nullptr)
    , m_pToolbar(nullptr)
    , m_pWorkingScene(nullptr)
    , m_eCurrentMode(EDITOR_MODE::NORMAL)
    , m_bShowUI(true)
    , m_vMousePos{}
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
    m_eCurrentMode = EDITOR_MODE::NORMAL;
    m_bShowUI = true;
    m_pSelectedObject = nullptr;
    m_bDragging = false;
    SetMapSize(Vec2(3840.f, 640.f));

    // 툴바 초기화 후 맵 크기 동기화
    if (m_pToolbar)
    {
        m_pToolbar->SetMapSize(m_vMapSize);
    }
}

void CEditorCore::Update()
{
    // Scene의 기본 오브젝트들 업데이트
    if (m_pWorkingScene)
    {
        m_pWorkingScene->CScene::Update();
    }

    // 스테이지 매니저 업데이트
    CStageMgr::GetInst()->Update();

    // 하위 시스템들 업데이트 (순서 중요)
    m_pCameraController->Update();      // 카메라 먼저
    m_pInput->Update();                 // 입력 처리
    m_pObjectManager->Update();         // 오브젝트 관리 (배경 업데이트 포함)
    m_pToolbar->Update();               // 툴바 업데이트
}

void CEditorCore::Render(HDC _dc)
{
    // 모든 렌더링을 에디터에서 처리 (순서 중요)

    // 배경 렌더링 (맨 뒤)
    if (m_pObjectManager->GetCurrentBackground())
    {
        m_pObjectManager->GetCurrentBackground()->Render(_dc);
    }

    // 스테이지 이미지 렌더링 (배경과 게임 객체 사이)
    CStageMgr::GetInst()->Render(_dc);

    // Scene의 모든 게임 오브젝트 렌더링 (타일, 몬스터, 아이템 등)
    if (m_pWorkingScene)
    {
        m_pWorkingScene->CScene::Render(_dc);
    }

    // 그리드 렌더링 (게임 오브젝트 위에)
    CGrid::GetInst()->Render(_dc);
    RenderMapBounds(_dc);

    // 에디터 전용 렌더링 (미리보기, 선택 박스 등)
    m_pRenderer->Render(_dc);

    // 툴바 렌더링 (UI보다 먼저)
    m_pToolbar->Render(_dc);

    // UI 렌더링 (맨 앞)
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

    switch (_eMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
        m_pObjectManager->ChangeObjectCategory(L"Monster");
        break;
    case EDITOR_MODE::PLACE_ITEM:
        m_pObjectManager->ChangeObjectCategory(L"Item");
        break;
    case EDITOR_MODE::PLACE_TILE:
        m_pObjectManager->ChangeObjectCategory(L"Tile");
        break;
    case EDITOR_MODE::PLACE_SPECIAL:
        m_pObjectManager->ChangeObjectCategory(L"Special");
        break;
    case EDITOR_MODE::PLACE_STAGE:
        // Stage Image 모드에서는 별도 카테고리 필요 없음
        break;
    }

    // 모드 변경 시 선택 해제
    if (_eMode != EDITOR_MODE::SELECT)
    {
        DeselectObject();
    }
}

const wchar_t* CEditorCore::GetModeString() const
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::NORMAL:         return L"Normal";
    case EDITOR_MODE::PLACE_MONSTER: return L"Place Monster";
    case EDITOR_MODE::PLACE_ITEM:   return L"Place Item";
    case EDITOR_MODE::PLACE_TILE:   return L"Place Tile";
    case EDITOR_MODE::PLACE_SPECIAL: return L"Place Special";
    case EDITOR_MODE::PLACE_STAGE:  return L"Stage Image";
    case EDITOR_MODE::SELECT:       return L"Select";
    case EDITOR_MODE::ERASE:        return L"Erase";
    case EDITOR_MODE::BACKGROUND:   return L"Background";
    case EDITOR_MODE::PLAYER_SPAWN: return L"Player Spawn";
    default:                        return L"Unknown";
    }
}

void CEditorCore::DeselectObject()
{
    m_pSelectedObject = nullptr;
    m_bDragging = false;
}

void CEditorCore::SetMapSize(Vec2 vSize)
{
    m_vMapSize = vSize;

    // 카메라 경계 설정
    if (m_pCameraController)
    {
        Vec2 vMin = Vec2(0.f, 0.f);
        Vec2 vMax = vSize;
        m_pCameraController->SetCameraBounds(vMin, vMax);
    }

    // 그리드에 맵 크기 알림
    if (CGrid::GetInst())
    {
        // CGrid에 맵 크기 설정 메서드가 있다면
        // CGrid::GetInst()->SetMapBounds(Vec2(0.f, 0.f), vSize);
    }
}

void CEditorCore::RenderMapBounds(HDC _dc)
{
    if (!m_pWorkingScene) return;

    // 카메라 변환 적용
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vOffset = vResolution / 2.f - vCameraPos;

    // 실제 좌표계로 맵 경계선 그리기
    HPEN hPen = CreatePen(PS_DASH, 2, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 맵 경계 사각형: (0,0) ~ (width, height)
    int left = (int)(0 + vOffset.x);
    int right = (int)(m_vMapSize.x + vOffset.x);
    int top = (int)(0 + vOffset.y);
    int bottom = (int)(m_vMapSize.y + vOffset.y);

    // 경계선 그리기
    MoveToEx(_dc, left, top, nullptr);
    LineTo(_dc, right, top);
    LineTo(_dc, right, bottom);
    LineTo(_dc, left, bottom);
    LineTo(_dc, left, top);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);

    // 맵 크기 텍스트 표시
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(255, 0, 0));

    wchar_t szMapSize[128];
    swprintf_s(szMapSize, L"Map Size: %.0fx%.0f", m_vMapSize.x, m_vMapSize.y);

    TextOut(_dc, left + 10, top + 10, szMapSize, (int)wcslen(szMapSize));
}
