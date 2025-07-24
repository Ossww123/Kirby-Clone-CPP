#include "pch.h"
#include "CEventMgr.h"

#include "CObject.h"
#include "CSceneMgr.h"
#include "CScene.h"

#include "CCollider.h"

CEventMgr::CEventMgr()
{
}

CEventMgr::~CEventMgr()
{
}

void CEventMgr::init()
{
    // 초기화할 내용이 있다면 여기에
}

void CEventMgr::update()
{
    // 1. 이전 프레임에서 삭제 예정된 오브젝트들 정리
    ClearGarbageObject();

    // 2. 현재 프레임의 모든 이벤트 처리
    for (size_t i = 0; i < m_vecEvent.size(); ++i)
    {
        Execute(m_vecEvent[i]);
    }

    // 3. 처리된 이벤트들 클리어
    m_vecEvent.clear();
}

void CEventMgr::Execute(tEvent& _event)
{
    switch (_event.eType)
    {
    case EVENT_TYPE::CREATE_OBJECT:
    {
        // wParam: GROUP_TYPE, lParam: CObject*
        CObject* pObj = (CObject*)_event.lParam;
        GROUP_TYPE eType = (GROUP_TYPE)_event.wParam;

        CSceneMgr::GetInst()->GetCurScene()->AddObject(pObj, eType);
    }
    break;

    case EVENT_TYPE::DELETE_OBJECT:
    {
        // lParam: CObject*
        CObject* pObj = (CObject*)_event.lParam;
        pObj->SetDead();

        // 가비지 컬렉션에 추가 (다음 프레임에 실제 삭제)
        m_vecGarbage.push_back(pObj);
    }
    break;

    case EVENT_TYPE::SCENE_CHANGE:
    {
        // lParam: SCENE_TYPE
        SCENE_TYPE eNextScene = (SCENE_TYPE)_event.lParam;
        CSceneMgr::GetInst()->ChangeScene(eNextScene);
    }
    break;

    case EVENT_TYPE::COLLISION_ENTER:
    {
        // wParam: CCollider*, lParam: CCollider*
        CCollider* pCol1 = (CCollider*)_event.wParam;
        CCollider* pCol2 = (CCollider*)_event.lParam;

        // 실제 충돌 콜백 호출
        pCol1->GetOwner()->OnCollisionEnter(pCol2);
        pCol2->GetOwner()->OnCollisionEnter(pCol1);
    }
    break;

    case EVENT_TYPE::COLLISION_EXIT:
    {
        // wParam: CCollider*, lParam: CCollider*
        CCollider* pCol1 = (CCollider*)_event.wParam;
        CCollider* pCol2 = (CCollider*)_event.lParam;

        // 실제 충돌 종료 콜백 호출
        pCol1->GetOwner()->OnCollisionExit(pCol2);
        pCol2->GetOwner()->OnCollisionExit(pCol1);
    }
    break;
    }
}

void CEventMgr::ClearGarbageObject()
{
    for (size_t i = 0; i < m_vecGarbage.size(); ++i)
    {
        delete m_vecGarbage[i];
    }
    m_vecGarbage.clear();
}