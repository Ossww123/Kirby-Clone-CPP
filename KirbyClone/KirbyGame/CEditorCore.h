#pragma once
#include "CObjectFactory.h"

// 전방 선언
class CEditorUI;
class CEditorInput;
class CEditorRenderer;
class CEditorFileManager;
class CEditorObjectManager;
class CEditorCameraController;
class CEditorToolbar;
class CScene;
class CObject;

class CEditorCore
{
public:
    // === 생명주기 함수 ===
    CEditorCore();
    ~CEditorCore();

    void Initialize(CScene* _pScene);
    void Update();
    void Render(HDC _dc);
    void Shutdown();

public:
    // === 모드 관리 ===
    void ChangeMode(EDITOR_MODE _eMode);
    EDITOR_MODE GetCurrentMode() const { return m_eCurrentMode; }
    const wchar_t* GetModeString() const;

public:
    // === 마우스 관리 ===
    void SetMousePos(Vec2 _vPos) { m_vMousePos = _vPos; }
    Vec2 GetMousePos() const { return m_vMousePos; }

public:
    // === 선택 관리 ===
    void SetSelectedObject(CObject* _pObj) { m_pSelectedObject = _pObj; };
    CObject* GetSelectedObject() const { return m_pSelectedObject; }
    void DeselectObject();
    void SetDragging(bool _bDrag) { m_bDragging = _bDrag; }
    bool IsDragging() const { return m_bDragging; }
    void SetDragStartPos(Vec2 _vPos) { m_vDragStartPos = _vPos; }
    Vec2 GetDragStartPos() const { return m_vDragStartPos; }

public:
    // === UI 관리 ===
    void SetShowUI(bool _bShow) { m_bShowUI = _bShow; }
    bool IsShowUI() const { return m_bShowUI; }

public:
    // === 맵 크기 관리 ===
    void SetMapSize(Vec2 _vSize);
    Vec2 GetMapSize() const { return m_vMapSize; }
    void RenderMapBounds(HDC _dc);

public:
    // === 접근자 함수들 ===
    CScene* GetWorkingScene() const { return m_pWorkingScene; }

    // 하위 시스템 접근자
    CEditorUI* GetUI() const { return m_pUI; }
    CEditorInput* GetInput() const { return m_pInput; }
    CEditorRenderer* GetRenderer() const { return m_pRenderer; }
    CEditorFileManager* GetFileManager() const { return m_pFileManager; }
    CEditorObjectManager* GetObjectManager() const { return m_pObjectManager; }
    CEditorCameraController* GetCameraController() const { return m_pCameraController; }
    CEditorToolbar* GetToolbar() const { return m_pToolbar; }

private:
    // === 하위 시스템들 ===
    CEditorUI* m_pUI;               // UI 관리 시스템
    CEditorInput* m_pInput;            // 입력 처리 시스템
    CEditorRenderer* m_pRenderer;         // 렌더링 시스템
    CEditorFileManager* m_pFileManager;      // 파일 관리 시스템
    CEditorObjectManager* m_pObjectManager;    // 오브젝트 관리 시스템
    CEditorCameraController* m_pCameraController; // 카메라 제어 시스템
    CEditorToolbar* m_pToolbar;          // 툴바 시스템

    // === 작업 환경 정보 ===
    CScene*     m_pWorkingScene;    // 현재 작업 중인 씬 (CScene_Tool에서 받아옴)
    Vec2        m_vMapSize;         // 맵 크기 정보

    // === 에디터 상태 ===
    EDITOR_MODE m_eCurrentMode;     // 현재 에디터 모드
    bool        m_bShowUI;          // UI 표시 여부

    // === 마우스 상태 ===
    Vec2        m_vMousePos;        // 마우스 위치

    // === 선택 및 드래그 상태 ===
    CObject*    m_pSelectedObject;  // 선택된 오브젝트
    bool        m_bDragging;        // 드래그 진행 여부
    Vec2        m_vDragStartPos;    // 드래그 시작 위치
};