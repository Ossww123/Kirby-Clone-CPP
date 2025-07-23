#pragma once

class CScene;

class CSceneMgr
{
    SINGLE(CSceneMgr);

private:
    CScene* m_pCurScene;    // 현재 활성화된 씬

public:
    void init();
    void update();
    void render(HDC _dc);

public:
    CScene* GetCurScene() { return m_pCurScene; }
};