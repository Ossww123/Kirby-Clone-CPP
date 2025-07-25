#include "pch.h"
#include "CObject.h"
#include "CCollider.h"
#include "CAnimator.h"
#include "CCamera.h"
#include "CTexture.h"

CObject::CObject()
	: m_vPos{}
	, m_vScale{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_bAlive(true)
	, m_pTex(nullptr)
{
}

CObject::~CObject()
{
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
}

void CObject::Render(HDC _dc)
{
	Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vPos);

	// 애니메이터가 있으면 애니메이션으로 렌더링
	if (nullptr != m_pAnimator)
	{
		m_pAnimator->Render(_dc);
	}
	// 텍스처가 있으면 텍스처로 렌더링
	else if (nullptr != m_pTex)
	{
		// 텍스처 크기 얻기
		UINT width = m_pTex->GetWidth();
		UINT height = m_pTex->GetHeight();

		// 오브젝트 중심에서 텍스처 그리기
		BitBlt(_dc,
			(int)(vRenderPos.x - width / 2.f),
			(int)(vRenderPos.y - height / 2.f),
			width, height,
			m_pTex->GetDC(),
			0, 0, SRCCOPY);
	}
	else
	{
		// 텍스처가 없으면 기본 사각형으로 렌더링
		Rectangle(_dc, (int)(vRenderPos.x - m_vScale.x / 2.f)
			, (int)(vRenderPos.y - m_vScale.y / 2.f)
			, (int)(vRenderPos.x + m_vScale.x / 2.f)
			, (int)(vRenderPos.y + m_vScale.y / 2.f));
	}

	// 충돌체가 있으면 충돌체도 렌더링
	if (nullptr != m_pCollider)
		m_pCollider->Render(_dc);
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