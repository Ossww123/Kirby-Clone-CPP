#include "pch.h"
#include "CCollisionMgr.h"

#include "CSceneMgr.h"
#include "CEventMgr.h"

#include "CScene.h"
#include "CObject.h"
#include "CCollider.h"

CCollisionMgr::CCollisionMgr()
    : m_arrCheck{}
{
}

CCollisionMgr::~CCollisionMgr()
{
}

void CCollisionMgr::init()
{
    // 충돌 체크할 그룹 설정
    CheckGroup(GROUP_TYPE::PLAYER, GROUP_TYPE::MONSTER);
    CheckGroup(GROUP_TYPE::PLAYER, GROUP_TYPE::TILE);
    CheckGroup(GROUP_TYPE::MONSTER, GROUP_TYPE::TILE);
    CheckGroup(GROUP_TYPE::PLAYER, GROUP_TYPE::SPECIAL);
    CheckGroup ( GROUP_TYPE::PLAYER , GROUP_TYPE::PROJ_MONSTER );
    CheckGroup ( GROUP_TYPE::MONSTER , GROUP_TYPE::PROJ_PLAYER );
}

void CCollisionMgr::update()
{
    // 등록된 그룹 간의 충돌 체크
    for (UINT iRow = 0; iRow < (UINT)GROUP_TYPE::END; ++iRow)
    {
        for (UINT iCol = iRow; iCol < (UINT)GROUP_TYPE::END; ++iCol)
        {
            if (m_arrCheck[iRow] & (1 << iCol))
            {
                CollisionGroupUpdate((GROUP_TYPE)iRow, (GROUP_TYPE)iCol);
            }
        }
    }
}

void CCollisionMgr::CheckGroup(GROUP_TYPE _eLeft, GROUP_TYPE _eRight)
{
    // 더 작은 값을 행으로, 큰 값을 열로 사용
    UINT iRow = (UINT)_eLeft;
    UINT iCol = (UINT)_eRight;

    if (iCol < iRow)
    {
        iRow = (UINT)_eRight;
        iCol = (UINT)_eLeft;
    }

    // 비트 연산으로 체크
    m_arrCheck[iRow] |= (1 << iCol);
}

void CCollisionMgr::UnCheckGroup(GROUP_TYPE _eLeft, GROUP_TYPE _eRight)
{
    // 더 작은 값을 행으로, 큰 값을 열로 사용
    UINT iRow = (UINT)_eLeft;
    UINT iCol = (UINT)_eRight;

    if (iCol < iRow)
    {
        iRow = (UINT)_eRight;
        iCol = (UINT)_eLeft;
    }

    // 비트 연산으로 체크 해제
    m_arrCheck[iRow] &= ~(1 << iCol);
}

void CCollisionMgr::CollisionGroupUpdate(GROUP_TYPE _eLeft, GROUP_TYPE _eRight)
{
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();

    const vector<CObject*>& vecLeft = pCurScene->GetGroupObject(_eLeft);
    const vector<CObject*>& vecRight = pCurScene->GetGroupObject(_eRight);

    map<ULONGLONG, bool>::iterator iter;

    // 모든 왼쪽 오브젝트와 오른쪽 오브젝트 간의 충돌 체크
    for (size_t i = 0; i < vecLeft.size(); ++i)
    {
        // 충돌체가 없으면 건너뛰기
        if (nullptr == vecLeft[i]->GetCollider())
            continue;

        for (size_t j = 0; j < vecRight.size(); ++j)
        {
            // 충돌체가 없거나 자기 자신이면 건너뛰기
            if (nullptr == vecRight[j]->GetCollider() || vecLeft[i] == vecRight[j])
                continue;

            CCollider* pLeftCol = vecLeft[i]->GetCollider();
            CCollider* pRightCol = vecRight[j]->GetCollider();

            // 두 충돌체 조합의 고유한 키값 생성
            COLLIDER_ID ID;
            ID.iLeft_id = pLeftCol->GetID();
            ID.iRight_id = pRightCol->GetID();

            iter = m_mapColInfo.find(ID.ID);

            // 충돌 정보가 없다면 등록
            if (m_mapColInfo.end() == iter)
            {
                m_mapColInfo.insert(make_pair(ID.ID, false));
                iter = m_mapColInfo.find(ID.ID);
            }

            // 현재 충돌 중인지 확인
            if (IsCollision(pLeftCol, pRightCol))
            {
                // 현재 충돌 중

                if (iter->second)
                {
                    // 이전에도 충돌 - 계속 충돌 중 (OnCollision)
                    // 둘 중 하나라도 죽을 예정이면 충돌 해제
                    if (vecLeft[i]->IsDead() || vecRight[j]->IsDead())
                    {
                        // 이벤트로 충돌 종료 처리
                        tEvent event(EVENT_TYPE::COLLISION_EXIT, (DWORD_PTR)pLeftCol, (DWORD_PTR)pRightCol);
                        CEventMgr::GetInst()->AddEvent(event);
                        iter->second = false;
                    }
                    else
                    {
                        // 계속 충돌 중 - 즉시 처리 (매 프레임 호출되어야 함)
                        pLeftCol->OnCollision(pRightCol);
                        pRightCol->OnCollision(pLeftCol);
                    }
                }
                else
                {
                    // 이전에는 충돌하지 않음 - 충돌 시작 (OnCollisionEnter)
                    // 둘 중 하나라도 죽을 예정이면 충돌하지 않음
                    if (!vecLeft[i]->IsDead() && !vecRight[j]->IsDead())
                    {
                        // 이벤트로 충돌 시작 처리
                        tEvent event(EVENT_TYPE::COLLISION_ENTER, (DWORD_PTR)pLeftCol, (DWORD_PTR)pRightCol);
                        CEventMgr::GetInst()->AddEvent(event);
                        iter->second = true;
                    }
                }
            }
            else
            {
                // 현재 충돌하지 않음

                if (iter->second)
                {
                    // 이전에는 충돌 - 충돌 해제 (OnCollisionExit)
                    tEvent event(EVENT_TYPE::COLLISION_EXIT, (DWORD_PTR)pLeftCol, (DWORD_PTR)pRightCol);
                    CEventMgr::GetInst()->AddEvent(event);
                    iter->second = false;
                }
            }
        }
    }
}

bool CCollisionMgr::IsCollision(CCollider* _pLeftCol, CCollider* _pRightCol)
{
    return _pLeftCol->IsCollision(_pRightCol);
}