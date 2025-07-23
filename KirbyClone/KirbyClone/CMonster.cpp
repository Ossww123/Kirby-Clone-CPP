#include "pch.h"
#include "CMonster.h"

#include "CTimeMgr.h"
#include "CCollider.h"

CMonster::CMonster()
    : m_vCenterPos{}
    , m_fSpeed(100.f)
    , m_fMaxDistance(50.f)
    , m_iDir(1)
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(40.f, 40.f));  // 몬스터는 더 작은 충돌 박스
}

CMonster::~CMonster()
{
}

void CMonster::Update()
{
    Vec2 vPos = GetPos();

    // 처음 업데이트 시 중심 위치 설정
    if (m_vCenterPos.x == 0.f && m_vCenterPos.y == 0.f)
    {
        m_vCenterPos = vPos;
    }

    // 좌우로 왕복 이동
    vPos.x += m_fSpeed * m_iDir * CTimeMgr::GetInst()->GetfDT();

    // 중심에서 최대 거리 이상 벗어나면 방향 전환
    float fDist = abs(m_vCenterPos.x - vPos.x);
    if (fDist >= m_fMaxDistance)
    {
        m_iDir *= -1;  // 방향 반전
    }

    SetPos(vPos);
}