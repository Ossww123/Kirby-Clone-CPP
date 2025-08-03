#pragma once

class CScene;

class CSceneMgr
{
    SINGLE(CSceneMgr);

private:
    CScene* m_arrScene[(UINT)SCENE_TYPE::END];  // 모든 씬 목록
    CScene* m_pCurScene;    // 현재 활성화된 씬

public:
    void init();
    void update();
    void render(HDC _dc);

private:
    void ChangeScene(SCENE_TYPE _eNext);

public:
    CScene* GetCurScene() { return m_pCurScene; }

    friend class CEventMgr;
};