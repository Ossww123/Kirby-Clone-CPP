#include "gamePCH.h"
#include "CObject.h"
#include "CCollider.h"
#include "CAnimator.h"
#include "CCamera.h"
#include "CRigidBody.h"
#include "CCore.h"
#include "CSceneMgr.h"
#include "CAnimation.h"
#include "CTimeMgr.h"

CObject::CObject()
	: m_vPos{}
	, m_vScale{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_pRigidBody(nullptr)
	, m_bAlive(true)
	, m_eObjectType(OBJECT_TYPE::END)
{
}

CObject::CObject(OBJECT_TYPE _eType)
	: m_vPos{}
	, m_vScale{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_pRigidBody(nullptr)
	, m_bAlive(true)
	, m_eObjectType(_eType)
{
}

CObject::~CObject()
{
    // 컴포넌트들 안전하게 해제
	if (nullptr != m_pCollider)
	{
		delete m_pCollider;
		m_pCollider = nullptr;
	}

	if (nullptr != m_pAnimator)
	{
		delete m_pAnimator;
		m_pAnimator = nullptr;
	}

	if (nullptr != m_pRigidBody)
	{
		delete m_pRigidBody;
		m_pRigidBody = nullptr;
	}
}

void CObject::Render(HDC _dc)
{
    // 렌더링 위치 계산
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vPos);

    // 스케일 팩터 결정
    float fScale = CCore::PIXEL_SCALE;

    // === 디버깅: 몬스터 렌더링 확인 ===
    if (m_eObjectType >= OBJECT_TYPE::MONSTER_WADDLE_DEE &&
        m_eObjectType <= OBJECT_TYPE::MONSTER_SPARKY)
    {
        static float debugTimer = 0.f;
        debugTimer += CTimeMgr::GetInst()->GetfDT();
        if (debugTimer >= 3.f)
        {
            debugTimer = 0.f;
        }
    }

    // 메인 렌더링 수행
    RenderMain(_dc, vRenderPos, fScale);

    // 콜라이더 렌더링
    RenderCollider(_dc);
}

void CObject::CreateCollider()
{
	m_pCollider = new CCollider;
	m_pCollider->m_pOwner = this;
}

void CObject::CreateAnimator()
{
	m_pAnimator = new CAnimator;
	m_pAnimator->m_pOwner = this;
}

void CObject::CreateRigidBody()
{
	m_pRigidBody = new CRigidBody;
	m_pRigidBody->m_pOwner = this;
}


void CObject::RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    // 우선순위: 애니메이터 → 기본 사각형
    if (nullptr != m_pAnimator)
    {
        RenderWithAnimator(_dc, _fScale);
    }
    else
    {
        RenderDefaultShape(_dc, _vRenderPos, _fScale);
    }
}

void CObject::RenderWithAnimator(HDC _dc, float _fScale)
{
    // 애니메이터의 스케일 렌더링 기능 활용
    m_pAnimator->RenderScaled(_dc, _fScale);
}


void CObject::RenderDefaultShape(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    // 스케일된 크기 계산
    Vec2 vScaledSize = m_vScale * _fScale;

    // 기본 사각형 렌더링
    Rectangle(_dc,
        (int)(_vRenderPos.x - vScaledSize.x / 2.f),
        (int)(_vRenderPos.y - vScaledSize.y / 2.f),
        (int)(_vRenderPos.x + vScaledSize.x / 2.f),
        (int)(_vRenderPos.y + vScaledSize.y / 2.f));
}


void CObject::RenderCollider(HDC _dc)
{
    if (nullptr != m_pCollider)
    {
        m_pCollider->Render(_dc);
    }
}
