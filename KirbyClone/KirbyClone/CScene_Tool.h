#pragma once

#include "CScene.h"

// 전방 선언
class CEditorCore;

class CScene_Tool : public CScene
{
public:
    CScene_Tool();
    virtual ~CScene_Tool();

public:
    // === 생명주기 함수 ===
    void Enter() override;
    void Exit() override;
    void Update() override;
    void Render(HDC _dc) override;

public:
    // === 레벨 파일 관리 ===
    void LoadLevel(const wstring& _strFileName);
    void SaveLevel(const wstring& _strFileName);
    void ClearLevel();

    // === 에디터 시스템 접근자 ===
    CEditorCore* GetEditorCore() const { return m_pEditorCore; }

    // === 씬 객체 관리 ===
    void ClearAllObjects() { DeleteAllObject(); }

private:
    // === 멤버 변수 ===
    CEditorCore* m_pEditorCore;  // 에디터 핵심 시스템
};