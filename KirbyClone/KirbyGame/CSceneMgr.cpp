#include "gamePCH.h"
#include "CSceneMgr.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"

#include "CCore.h"
#include "CScene.h"
#include "CScene_Start.h"
#include "CScene_Stage01.h"
#include "CScene_Stage02.h"

CSceneMgr::CSceneMgr()
    : m_pCurScene(nullptr)
    , m_arrScene{}
    , m_eCurSceneType(SCENE_TYPE::START)
{
}

CSceneMgr::~CSceneMgr()
{
    // 모든 씬 정리
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
    //m_arrScene[(UINT)SCENE_TYPE::TOOL] = new CScene_Tool;
    m_arrScene[(UINT)SCENE_TYPE::STAGE_01] = new CScene_Stage01;
    m_arrScene[(UINT)SCENE_TYPE::STAGE_02] = new CScene_Stage02;

    // 시작 씬 설정
    m_eCurSceneType = SCENE_TYPE::START;
    m_pCurScene = m_arrScene[(UINT)m_eCurSceneType];

    if (m_pCurScene)
    {
        m_pCurScene->Enter();
    }
}

void CSceneMgr::update()
{
    // 현재 씬 업데이트
    if (m_pCurScene)
    {
        m_pCurScene->Update();
    }

    // 전역 씬 전환 키 처리
    HandleGlobalSceneTransition();
}

void CSceneMgr::render(HDC _dc)
{
    // 현재 씬 렌더링
    if (m_pCurScene)
    {
        m_pCurScene->Render(_dc);
    }
}

void CSceneMgr::ChangeScene(SCENE_TYPE _eNext)
{
    if (m_pCurScene) m_pCurScene->Exit();

    m_eCurSceneType = _eNext;
    m_pCurScene = m_arrScene[(UINT)_eNext];

    if (m_pCurScene) m_pCurScene->Enter();
}

void CSceneMgr::HandleGlobalSceneTransition()
{
    // STAGE_01로 이동 (Ctrl+1)
    if (KEY_TAP(KEY::ALPHA_1) && KEY_HOLD(KEY::CTRL))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::STAGE_01);
        CEventMgr::GetInst()->AddEvent(event);
    }

    // STAGE_02로 이동 (Ctrl+2)
    if (KEY_TAP(KEY::ALPHA_2) && KEY_HOLD(KEY::CTRL))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::STAGE_02);
        CEventMgr::GetInst()->AddEvent(event);
    }
}