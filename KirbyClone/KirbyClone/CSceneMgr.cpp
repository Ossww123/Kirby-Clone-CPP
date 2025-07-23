#include "pch.h"
#include "CSceneMgr.h"

#include "CScene.h"
#include "CScene_Start.h"

CSceneMgr::CSceneMgr()
    : m_pCurScene(nullptr)
{
}

CSceneMgr::~CSceneMgr()
{
    // ÇöÀç ¾À »èÁ¦
    if (nullptr != m_pCurScene)
        delete m_pCurScene;
}

void CSceneMgr::init()
{
    // ½ÃÀÛ ¾À »ı¼º
    m_pCurScene = new CScene_Start;
    m_pCurScene->Enter();
}

void CSceneMgr::update()
{
    m_pCurScene->Update();
}

void CSceneMgr::render(HDC _dc)
{
    m_pCurScene->Render(_dc);
}