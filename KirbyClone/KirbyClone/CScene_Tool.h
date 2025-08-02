#pragma once
#include "CScene.h"

// Editor 관련 클래스들
class CEditorCore;

class CScene_Tool : public CScene
{
private:
    CEditorCore* m_pEditorCore;  // 에디터 핵심 시스템

public:
    virtual void Enter() override;
    virtual void Exit() override;
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // 에디터 접근자
    CEditorCore* GetEditorCore() { return m_pEditorCore; }

    // 기본 Scene 기능은 유지
    void LoadLevel(const wstring& _strFileName);
    void SaveLevel(const wstring& _strFileName);
    void ClearLevel();

    void ClearAllObjects() { DeleteAllObject(); }

public:
    CScene_Tool();
    virtual ~CScene_Tool();
};