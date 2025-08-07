#pragma once
#include "CObjectFactory.h"

// 전방 선언
class CEditorUI;
class CEditorInput;
class CEditorRenderer;
class CEditorFileManager;
class CEditorObjectManager;
class CEditorCameraController;
class CScene;
class CEditorToolbar;

class CEditorCore
{
private:
    // 에디터 하위 시스템들
    CEditorUI* m_pUI;
    CEditorInput* m_pInput;
    CEditorRenderer* m_pRenderer;
    CEditorFileManager* m_pFileManager;
    CEditorObjectManager* m_pObjectManager;
    CEditorCameraController* m_pCameraController;
    CEditorToolbar* m_pToolbar;

    // 현재 작업 중인 씬 (CScene_Tool에서 받아옴)
    CScene* m_pWorkingScene;

    // 에디터 상태
    EDITOR_MODE         m_eCurrentMode;
    bool                m_bShowUI;
    Vec2                m_vMousePos;
    bool                m_bMouseClick;
    float               m_fClickTime;

    // 선택된 오브젝트 관련
    CObject* m_pSelectedObject;
    bool                m_bDragging;
    Vec2                m_vDragStartPos;

    Vec2 m_vMapSize;

public:
    void Initialize(CScene* _pScene);
    void Update();
    void Render(HDC _dc);
    void Shutdown();

    // 모드 관리
    void ChangeMode(EDITOR_MODE _eMode);
    EDITOR_MODE GetCurrentMode() { return m_eCurrentMode; }
    const wchar_t* GetModeString();

    // 마우스 관련
    void SetMousePos(Vec2 _vPos) { m_vMousePos = _vPos; }
    Vec2 GetMousePos() { return m_vMousePos; }
    void SetMouseClick(bool _bClick, float _fTime = 0.5f);
    bool IsMouseClick() { return m_bMouseClick; }
    void UpdateClickTime(float _fDT);

    // 선택 관련
    void SetSelectedObject(CObject* _pObj);
    CObject* GetSelectedObject() { return m_pSelectedObject; }
    void DeselectObject();
    void SetDragging(bool _bDrag) { m_bDragging = _bDrag; }
    bool IsDragging() { return m_bDragging; }
    void SetDragStartPos(Vec2 _vPos) { m_vDragStartPos = _vPos; }
    Vec2 GetDragStartPos() { return m_vDragStartPos; }

    // UI 관련
    void SetShowUI(bool _bShow) { m_bShowUI = _bShow; }
    bool IsShowUI() { return m_bShowUI; }

    // 씬 접근
    CScene* GetWorkingScene() { return m_pWorkingScene; }

    // 하위 시스템 접근자
    CEditorUI* GetUI() { return m_pUI; }
    CEditorInput* GetInput() { return m_pInput; }
    CEditorRenderer* GetRenderer() { return m_pRenderer; }
    CEditorFileManager* GetFileManager() { return m_pFileManager; }
    CEditorObjectManager* GetObjectManager() { return m_pObjectManager; }
    CEditorCameraController* GetCameraController() { return m_pCameraController; }
    CEditorToolbar* GetToolbar() { return m_pToolbar; }

    // 맵 크기 관련
    void SetMapSize(Vec2 vSize);
    Vec2 GetMapSize() const { return m_vMapSize; }

    // 맵 경계 표시 (옵션)
    void RenderMapBounds(HDC _dc);

public:
    CEditorCore();
    ~CEditorCore();
};