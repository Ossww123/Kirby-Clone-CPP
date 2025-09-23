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
    : m_Transform{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_pRigidBody(nullptr)
	, m_bAlive(true)
	, m_eObjectType(OBJECT_TYPE::END)
{
}

CObject::CObject(OBJECT_TYPE _eType)
    : m_Transform{}
	, m_pCollider(nullptr)
	, m_pAnimator(nullptr)
	, m_pRigidBody(nullptr)
	, m_bAlive(true)
	, m_eObjectType(_eType)
{
}

CObject::~CObject()
{
    OnDestroy();

    if (m_pCollider) { delete m_pCollider;  m_pCollider = nullptr; }
    if (m_pAnimator) { delete m_pAnimator;  m_pAnimator = nullptr; }
    if (m_pRigidBody) { delete m_pRigidBody; m_pRigidBody = nullptr; }
}

void CObject::Render(HDC _dc)
{
    // 카메라 적용 + 픽셀 스케일 결정
    const Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_Transform.position);
    const float fScale = CCore::PIXEL_SCALE;

    OnPreRender(_dc);
    RenderMain(_dc, vRenderPos, fScale);
    OnPostRender(_dc);
}

void CObject::CreateCollider() {
    if (m_pCollider) { assert(false && "CreateCollider called twice"); return; }
    m_pCollider = new CCollider;
    m_pCollider->m_pOwner = this;
}

void CObject::CreateAnimator() {
    if (m_pAnimator) { assert(false && "CreateAnimator called twice"); return; }
    m_pAnimator = new CAnimator;
    m_pAnimator->m_pOwner = this;
}

void CObject::CreateRigidBody() {
    if (m_pRigidBody) { assert(false && "CreateRigidBody called twice"); return; }
    m_pRigidBody = new CRigidBody;
    m_pRigidBody->m_pOwner = this;
}



void CObject::RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    if (m_pAnimator)
    {
        // 애니메이터는 Transform의 flipX를 내부에서 참조하도록(규약)
        // 필요 시 여기서 m_pAnimator->SetFlipX(m_Transform.flipX); 등으로 동기화
        RenderWithAnimator(_dc, _fScale);
    }
#if defined(_DEBUG) && defined(ENABLE_DEBUG_DRAW_PRIMITIVES)
    else
    {
        // 실제 게임 릴리즈에서는 기본 사각형은 그리지 않음(디버그때만)
        RenderDefaultShape(_dc, _vRenderPos, _fScale);
    }
#endif

#if defined(_DEBUG) && defined(ENABLE_DEBUG_DRAW_COLLIDER)
    RenderCollider(_dc);
#endif
}

void CObject::RenderWithAnimator(HDC _dc, float _fScale)
{
    m_pAnimator->RenderScaled(_dc, _fScale);
}


void CObject::RenderDefaultShape(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    // 디버그 사각형: 스케일 적용 후 픽셀 스냅
    const Vec2 vScaledSize = Vec2{ m_Transform.scale.x * _fScale, m_Transform.scale.y * _fScale };

    const int l = static_cast<int>(_vRenderPos.x - vScaledSize.x * 0.5f);
    const int t = static_cast<int>(_vRenderPos.y - vScaledSize.y * 0.5f);
    const int r = static_cast<int>(_vRenderPos.x + vScaledSize.x * 0.5f);
    const int b = static_cast<int>(_vRenderPos.y + vScaledSize.y * 0.5f);

    Rectangle(_dc, l, t, r, b);
}


void CObject::RenderCollider(HDC _dc)
{
    if (m_pCollider)
        m_pCollider->Render(_dc);
}
