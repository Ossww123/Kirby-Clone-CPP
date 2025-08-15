#include "pch.h"
#include "CEventMgr.h"

#include "CObject.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"

#include "CCollider.h"
#include "CPlayerStateMachine.h"

CEventMgr::CEventMgr()
{
}

CEventMgr::~CEventMgr()
{
}

// === 플레이어 상태 변경 이벤트 헬퍼 함수 ===

void CEventMgr::RequestPlayerStateChange(CPlayer* _pPlayer, PLAYER_STATE _eNewState)
{
    tEvent event;
    event.eType = EVENT_TYPE::PLAYER_STATE_CHANGE;
    event.wParam = (DWORD_PTR)_eNewState;  // 새로운 상태
    event.lParam = (DWORD_PTR)_pPlayer;    // 플레이어 포인터

    GetInst()->AddEvent(event);
}

void CEventMgr::init()
{
    // 초기화할 내용이 있다면 여기에
}

void CEventMgr::update()
{
    // 1. 이전 프레임에서 삭제 예정된 오브젝트들 정리
    ClearGarbageObject();

    // 2. 플레이어 상태 변경 요청 맵 초기화
    m_mapPlayerStateRequests.clear();

    // 3. 현재 프레임의 모든 이벤트 처리
    for (size_t i = 0; i < m_vecEvent.size(); ++i)
    {
        Execute(m_vecEvent[i]);
    }

    // 4. 플레이어 상태 변경 요청들을 우선순위에 따라 처리
    ProcessPlayerStateRequests();

    // 5. 처리된 이벤트들 클리어
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

    case EVENT_TYPE::PLAYER_STATE_CHANGE:
        ExecutePlayerStateChange(_event);
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

void CEventMgr::ExecutePlayerStateChange(tEvent& _event)
{
    PLAYER_STATE eNewState = (PLAYER_STATE)_event.wParam;
    CPlayer* pPlayer = (CPlayer*)_event.lParam;

    if (!pPlayer)
        return;

    // 기존에 요청된 상태와 비교하여 더 높은 우선순위만 저장
    auto iter = m_mapPlayerStateRequests.find(pPlayer);
    if (iter != m_mapPlayerStateRequests.end())
    {
        // 이미 요청된 상태가 있음 - 우선순위 비교
        PLAYER_STATE existingState = iter->second;
        int existingPriority = GetStatePriority(existingState);
        int newPriority = GetStatePriority(eNewState);

        if (newPriority > existingPriority)
        {
            // 새로운 상태가 더 높은 우선순위
            iter->second = eNewState;
        }
    }
    else
    {
        // 첫 번째 요청
        m_mapPlayerStateRequests[pPlayer] = eNewState;
    }
}

void CEventMgr::ProcessPlayerStateRequests()
{
    // 각 플레이어별로 최고 우선순위 상태로 변경
    for (auto& pair : m_mapPlayerStateRequests)
    {
        CPlayer* pPlayer = pair.first;
        PLAYER_STATE eState = pair.second;

        if (pPlayer && !pPlayer->IsDead())
        {
            CPlayerStateMachine* pStateMachine = pPlayer->GetStateMachine();
            if (pStateMachine)
            {
                pStateMachine->ChangeStateInternal(eState);
            }
        }
    }
}

int CEventMgr::GetStatePriority(PLAYER_STATE _eState) const
{
    // 우선순위: 높을수록 중요함 (0이 가장 낮음)
    switch (_eState)
    {
        // === 최고 우선순위: 생명 관련 ===
    case PLAYER_STATE::END:          return 1000;  // 게임오버 등

        // === 높은 우선순위: 피격/특수 상태 ===
        // (DAMAGE 상태가 있다면 여기에 추가)
    case PLAYER_STATE::SWALLOW:      return 900;   // 삼키기 (중단 불가)
    case PLAYER_STATE::EXHALE:       return 900;   // 내뱉기 (중단 불가)

        // === 중간 우선순위: 능력 사용 ===
    case PLAYER_STATE::INHALE_READY: return 700;
    case PLAYER_STATE::INHALE_1:     return 700;
    case PLAYER_STATE::INHALE_2:     return 700;
    case PLAYER_STATE::INHALE_HOLD:  return 700;

        // === 중간 우선순위: 공중 동작 ===
    case PLAYER_STATE::JUMP:         return 600;
    case PLAYER_STATE::FALL:         return 600;
    case PLAYER_STATE::FALL2:        return 600;
    case PLAYER_STATE::BOUNCE:       return 600;
    case PLAYER_STATE::MOUTHFUL_JUMP: return 600;

        // === 중간 우선순위: 특수 동작 ===
    case PLAYER_STATE::SLIDE:        return 500;   // 슬라이드
    case PLAYER_STATE::CROUCH:       return 400;   // 크라우치

        // === 낮은 우선순위: 이동 상태 ===
    case PLAYER_STATE::RUN:          return 300;
    case PLAYER_STATE::MOUTHFUL_RUN: return 300;
    case PLAYER_STATE::WALK:         return 200;
    case PLAYER_STATE::MOUTHFUL_WALK: return 200;

        // === 가장 낮은 우선순위: 대기 상태 ===
    case PLAYER_STATE::IDLE:         return 100;
    case PLAYER_STATE::MOUTHFUL_IDLE: return 100;

    default:                         return 0;     // 알 수 없는 상태
    }
}