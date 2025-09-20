#include "gamePCH.h"
#include "CRigidBody.h"
#include "CObject.h"
#include "CTimeMgr.h"

// 전역 중력값 초기화 (980은 픽셀/초^2 단위)
float CRigidBody::s_fGravity = 980.f;

CRigidBody::CRigidBody()
    : m_pOwner(nullptr)
    , m_vVelocity{}
    , m_vAccel{}
    , m_vForce{}
    , m_fMass(1.f)
    , m_fGravityScale(1.f)
    , m_fMaxVelocity(2000.f)
    , m_fFriction(0.1f)
    , m_bUseGravity(true)
    , m_bGround(false)
{
}

CRigidBody::~CRigidBody()
{
}

void CRigidBody::AddForce(Vec2 _vForce)
{
    // F = ma, a = F/m
    m_vForce += _vForce;
}

void CRigidBody::AddVelocity(Vec2 _vVelocity)
{
    m_vVelocity += _vVelocity;
}

void CRigidBody::SetVelocityX(float _fVelX)
{
    m_vVelocity.x = _fVelX;
}

void CRigidBody::SetVelocityY(float _fVelY)
{
    m_vVelocity.y = _fVelY;
}

void CRigidBody::Update()
{
    float fDT = CTimeMgr::GetInst()->GetfDT();

    // 1. 힘을 가속도로 변환 (F = ma, a = F/m)
    m_vAccel = m_vForce / m_fMass;

    // 2. 중력 적용
    if (m_bUseGravity && !m_bGround)
    {
        m_vAccel.y += s_fGravity * m_fGravityScale;
    }

    // 3. 가속도를 속도에 적용 (v = v0 + at)
    m_vVelocity += m_vAccel * fDT;

    // 4. 마찰력 적용 (바닥에 있을 때만)
    if (m_bGround)
    {
        // X축 마찰
        if (abs(m_vVelocity.x) > 0.1f)
        {
            float fFrictionForce = m_fFriction * s_fGravity * fDT;
            if (m_vVelocity.x > 0)
                m_vVelocity.x = max(0.f, m_vVelocity.x - fFrictionForce);
            else
                m_vVelocity.x = min(0.f, m_vVelocity.x + fFrictionForce);
        }
        else
        {
            m_vVelocity.x = 0.f;
        }
    }

    // 5. 최대 속도 제한
    float fSpeed = m_vVelocity.Length();
    if (fSpeed > m_fMaxVelocity)
    {
        m_vVelocity.Normalize();
        m_vVelocity *= m_fMaxVelocity;
    }

    // 6. 속도를 위치에 적용 (s = s0 + vt)
    if (nullptr != m_pOwner)
    {
        Vec2 vPos = m_pOwner->GetPos();
        vPos += m_vVelocity * fDT;
        m_pOwner->SetPos(vPos);
    }

    // 7. 이번 프레임 힘 초기화
    m_vForce = Vec2(0.f, 0.f);
}