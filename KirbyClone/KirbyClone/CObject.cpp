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
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vPos);

    // 현재 씬이 툴 씬이 아닌 경우에만 4배 스케일 적용
    SCENE_TYPE currentScene = CSceneMgr::GetInst()->GetCurSceneType();
    float fScale = (currentScene == SCENE_TYPE::TOOL) ? 1.0f : CCore::GetPixelScale();

    // 애니메이터가 있으면 애니메이션으로 렌더링
    if (nullptr != m_pAnimator)
    {
        if (fScale > 1.0f)
        {
            // 4배 확대 렌더링 (게임 씬)
            // CAnimator의 RenderScaled 함수 사용
            m_pAnimator->RenderScaled(_dc, fScale);
        }
        else
        {
            // 원본 크기 렌더링 (툴 씬)
            m_pAnimator->Render(_dc);
        }
    }
    // 텍스처가 있으면 텍스처로 렌더링
    else if (nullptr != m_pTex)
    {
        UINT width = m_pTex->GetWidth();
        UINT height = m_pTex->GetHeight();

        if (fScale > 1.0f)
        {
            // 4배 확대 렌더링 (게임 씬)
            StretchBlt(_dc,
                (int)(vRenderPos.x - (width * fScale) / 2.f),
                (int)(vRenderPos.y - (height * fScale) / 2.f),
                (int)(width * fScale),
                (int)(height * fScale),
                m_pTex->GetDC(),
                0, 0, width, height, SRCCOPY);
        }
        else
        {
            // 원본 크기 렌더링 (툴 씬)
            BitBlt(_dc,
                (int)(vRenderPos.x - width / 2.f),
                (int)(vRenderPos.y - height / 2.f),
                width, height,
                m_pTex->GetDC(),
                0, 0, SRCCOPY);
        }
    }
    else
    {
        // 기본 사각형 렌더링
        Vec2 vScaledSize = m_vScale * fScale;
        Rectangle(_dc,
            (int)(vRenderPos.x - vScaledSize.x / 2.f),
            (int)(vRenderPos.y - vScaledSize.y / 2.f),
            (int)(vRenderPos.x + vScaledSize.x / 2.f),
            (int)(vRenderPos.y + vScaledSize.y / 2.f));
    }

    // 충돌체 렌더링
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

void CObject::CreateRigidBody()
{
	m_pRigidBody = new CRigidBody;
	m_pRigidBody->m_pOwner = this;
}
