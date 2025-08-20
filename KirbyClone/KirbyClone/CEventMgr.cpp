#include "pch.h"
#include "CEventMgr.h"

#include "CObject.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CPlayerHealthSystem.h"
#include "CMonster.h"
#include "CBasicMonster.h"

#include "CCollider.h"
#include "CPlayerStateMachine.h"
#include "CProjectile.h"

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
    // 이전 프레임에서 삭제 예정된 오브젝트들 정리
    ClearGarbageObject();

    // 현재 프레임의 모든 이벤트 처리
    for (size_t i = 0; i < m_vecEvent.size(); ++i)
    {
        Execute(m_vecEvent[i]);
    }

    // 처리된 이벤트들 클리어
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

    case EVENT_TYPE::PLAYER_DAMAGE:
        ExecutePlayerDamage(_event);
        break;

    case EVENT_TYPE::PLAYER_SLIDE_KICK_RECOIL:
        ExecutePlayerSlideKickRecoil(_event);
        break;

    case EVENT_TYPE::MONSTER_DAMAGE:
        ExecuteMonsterDamage(_event);
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

void CEventMgr::ExecutePlayerDamage(tEvent& _event)
{
    CPlayer* pPlayer = (CPlayer*)_event.wParam;
    Vec2* pKnockbackDir = (Vec2*)_event.lParam;

    if (!pPlayer || pPlayer->IsDead())
    {
        // 메모리 정리
        if (pKnockbackDir)
            delete pKnockbackDir;
        return;
    }

    // 무적 상태 체크 (이중 체크)
    if (pPlayer->GetHealthSystem() &&
        pPlayer->GetHealthSystem()->IsInvincible())
    {
        if (pKnockbackDir)
            delete pKnockbackDir;
        return;
    }

    // === 체력 시스템에서만 데미지 처리 (무적시간, 넉백 등) ===
    // 상태 변경은 PLAYER_STATE_CHANGE 이벤트에서 별도 처리
    Vec2 knockbackDir = pKnockbackDir ? *pKnockbackDir : Vec2(0.f, 0.f);
    if (pPlayer->GetHealthSystem())
    {
        pPlayer->GetHealthSystem()->TakeDamage(1, knockbackDir);
    }

    // 메모리 정리
    if (pKnockbackDir)
        delete pKnockbackDir;
}

void CEventMgr::ExecutePlayerSlideKickRecoil(tEvent& _event)
{
    CProjectile* pProjectile = (CProjectile*)_event.wParam;
    CObject* pMonster = (CObject*)_event.lParam;
    
    if (!pProjectile || !pMonster)
        return;

    // 플레이어 찾기
    CPlayer* pPlayer = nullptr;
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& playerObjs = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!playerObjs.empty())
        {
            pPlayer = dynamic_cast<CPlayer*>(playerObjs[0]);
        }
    }

    if (!pPlayer)
        return;

    // 플레이어에게 슬라이드킥 반동 요청
    pPlayer->RequestSlideKickRecoil();
}

void CEventMgr::ExecuteMonsterDamage(tEvent& _event)
{
    CMonster* pMonster = (CMonster*)_event.wParam;

    if (!pMonster || pMonster->IsDead())
        return;

    // === CBasicMonster인지 확인하고 넉백 정보 전달 ===
    CBasicMonster* pBasicMonster = dynamic_cast<CBasicMonster*>(pMonster);
    if (pBasicMonster && _event.lParam != 0)
    {
        // 데미지 소스 객체에서 위치 정보 가져오기
        CObject* pDamageSource = (CObject*)_event.lParam;
        Vec2 vSourcePos = pDamageSource->GetPos();
        
        // 몬스터의 TakeDamage에서 넉백 방향 계산을 위해 데미지 소스 위치 설정
        pBasicMonster->SetDamageSourcePos(vSourcePos);
    }

    // === 몬스터 데미지 처리 ===
    pMonster->TakeDamage();

    // 몬스터의 경우 TakeDamage()에서 내부적으로 DAMAGE 상태로 전환
    // 또는 추가적인 상태 전환이 필요하다면 여기서 처리
}