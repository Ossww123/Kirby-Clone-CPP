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
    , m_bAlive(true)
    , m_ObjectType(OBJECT_TYPE::END)
    , m_Group(GROUP_TYPE::DEFAULT)
{
}

CObject::CObject(OBJECT_TYPE _eType)
    : m_Transform{}
    , m_bAlive(true)
    , m_ObjectType(_eType)
    , m_Group(GROUP_TYPE::DEFAULT)
{
}

CObject::~CObject()
{
    OnDestroy();
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

void CObject::InitOnce()
{
    if (m_bInitialized) return;
    Init();
    m_bInitialized = true;
}

void CObject::CreateCollider() {
    if (m_pCollider) { assert(false && "CreateCollider called twice"); return; }
    m_pCollider = std::make_unique<CCollider>();
    m_pCollider->m_pOwner = this;
}

void CObject::CreateAnimator() {
    if (m_pAnimator) { assert(false && "CreateAnimator called twice"); return; }
    m_pAnimator = std::make_unique<CAnimator>();
    m_pAnimator->m_pOwner = this;
}

void CObject::CreateRigidBody() {
    if (m_pRigidBody) { assert(false && "CreateRigidBody called twice"); return; }
    m_pRigidBody = std::make_unique<CRigidBody>();
    m_pRigidBody->m_pOwner = this;
}

void CObject::RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale)
{
    if (m_pAnimator)
        RenderWithAnimator(_dc, _fScale);
#if defined(_DEBUG) && defined(ENABLE_DEBUG_DRAW_PRIMITIVES)
    else
        RenderDefaultShape(_dc, _vRenderPos, _fScale);
#endif

#if defined(_DEBUG) && defined(ENABLE_DEBUG_DRAW_COLLIDER)
    RenderCollider(_dc, _fScale);   // ← 변경
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


void CObject::RenderCollider(HDC _dc, float _fScale)
{
    if (m_pCollider)
        m_pCollider->RenderScaled(_dc, _fScale);
}
