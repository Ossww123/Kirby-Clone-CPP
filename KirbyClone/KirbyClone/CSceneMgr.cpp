#include "pch.h"
#include "CSceneMgr.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"

#include "CScene.h"
#include "CScene_Start.h"
#include "CScene_Tool.h"

CSceneMgr::CSceneMgr()
    : m_pCurScene(nullptr)
    , m_arrScene{}
{
}

CSceneMgr::~CSceneMgr()
{
    // 모든 씬 삭제
    for (UINT i = 0; i < (UINT)SCENE_TYPE::END; ++i)
    {
        if (nullptr != m_arrScene[i])
            delete m_arrScene[i];
    }
}

void CSceneMgr::init()
{
    // 모든 씬 생성
    m_arrScene[(UINT)SCENE_TYPE::START] = new CScene_Start;
    m_arrScene[(UINT)SCENE_TYPE::TOOL] = new CScene_Tool;

    // 시작 씬 설정
    m_pCurScene = m_arrScene[(UINT)SCENE_TYPE::START];
    m_pCurScene->Enter();
}

void CSceneMgr::update()
{
    m_pCurScene->Update();

    // 씬 전환 체크 (T키)
    if (KEY_TAP(KEY::T))
    {
        if (m_pCurScene == m_arrScene[(UINT)SCENE_TYPE::START])
        {
            // START 씬에서 TOOL 씬으로 전환 이벤트 발생
            tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::TOOL);
            CEventMgr::GetInst()->AddEvent(event);
        }
        else
        {
            // TOOL 씬에서 START 씬으로 전환 이벤트 발생  
            tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::START);
            CEventMgr::GetInst()->AddEvent(event);
        }
    }
}

void CSceneMgr::render(HDC _dc)
{
    m_pCurScene->Render(_dc);
}

void CSceneMgr::ChangeScene(SCENE_TYPE _eNext)
{
    m_pCurScene->Exit();
    m_pCurScene = m_arrScene[(UINT)_eNext];
    m_pCurScene->Enter();
}