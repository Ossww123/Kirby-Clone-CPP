#include "pch.h"
#include "CObject.h"
#include "CCollider.h"
#include "CAnimator.h"
#include "CCamera.h"
#include "CTexture.h"
#include "CRigidBody.h"
#include "CCore.h"
#include "CSceneMgr.h"
#include "CAnimation.h"

CObject::CObject()
	: m_vPos{}
	, m_vScale{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_pRigidBody(nullptr)
	, m_bAlive(true)
	, m_pTex(nullptr)
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
	, m_pTex(nullptr)
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
    float fScale = GetRenderScale();

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

float CObject::GetRenderScale() const
{
    return 0.0f;
}

void CObject::RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    // 우선순위: 애니메이터 → 텍스처 → 기본 사각형
    if (nullptr != m_pAnimator)
    {
        RenderWithAnimator(_dc, _fScale);
    }
    else if (nullptr != m_pTex)
    {
        RenderWithTexture(_dc, _vRenderPos, _fScale);
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

void CObject::RenderWithTexture(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    // 텍스처 정보 가져오기
    UINT width = m_pTex->GetWidth();
    UINT height = m_pTex->GetHeight();

    // 스케일된 크기 계산
    int scaledWidth = (int)(width * _fScale);
    int scaledHeight = (int)(height * _fScale);

    // 렌더링 위치 계산 (중앙 기준)
    int renderX = (int)(_vRenderPos.x - scaledWidth / 2.f);
    int renderY = (int)(_vRenderPos.y - scaledHeight / 2.f);

    // 스케일된 텍스처 렌더링
    StretchBlt(_dc,
        renderX, renderY,
        scaledWidth, scaledHeight,
        m_pTex->GetDC(),
        0, 0, width, height,
        SRCCOPY);
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
