#include "pch.h"
#include "CCollider.h"
#include "CObject.h"
#include "CCamera.h"
#include "CKeyMgr.h"

UINT CCollider::g_iNextID = 0;

CCollider::CCollider()
    : m_pOwner(nullptr)
    , m_vOffsetPos{}
    , m_vScale{}
    , m_iID(g_iNextID++)
{
}

CCollider::~CCollider()
{
}

Vec2 CCollider::GetFinalPos()
{
    Vec2 vObjectPos = m_pOwner->GetPos();
    return vObjectPos + m_vOffsetPos;
}

void CCollider::AddCollidingCollider(CCollider* _pOther)
{
    // 이미 목록에 있는지 확인
    for (CCollider* pCollider : m_vecCollidingColliders)
    {
        if (pCollider == _pOther)
            return; // 이미 존재함
    }

    // 목록에 추가
    m_vecCollidingColliders.push_back(_pOther);
}

void CCollider::RemoveCollidingCollider(CCollider* _pOther)
{
    // 벡터에서 해당 콜라이더 제거
    auto iter = std::find(m_vecCollidingColliders.begin(), m_vecCollidingColliders.end(), _pOther);
    if (iter != m_vecCollidingColliders.end())
    {
        m_vecCollidingColliders.erase(iter);
    }
}

bool CCollider::IsCollidingWith(CCollider* _pOther) const
{
    // 현재 충돌 중인지 확인
    for (CCollider* pCollider : m_vecCollidingColliders)
    {
        if (pCollider == _pOther)
            return true;
    }
    return false;
}

void CCollider::FinalUpdate()
{
    // 충돌체의 위치를 따라감
}

void CCollider::Render(HDC _dc)
{
    // 월드 좌표를 카메라 좌표로 변환
    Vec2 vPos = GetFinalPos();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 충돌체 시각화 (디버그용)
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 충돌체 펜
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    Rectangle(_dc, int(vRenderPos.x - m_vScale.x / 2.f)
        , int(vRenderPos.y - m_vScale.y / 2.f)
        , int(vRenderPos.x + m_vScale.x / 2.f)
        , int(vRenderPos.y + m_vScale.y / 2.f));

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

bool CCollider::IsCollision(CCollider* _pOther)
{
    // 사각형 충돌 검사 (AABB) - 월드 좌표에서 계산
    Vec2 vPos = GetFinalPos();
    Vec2 vOtherPos = _pOther->GetFinalPos();

    Vec2 vScale = GetScale();
    Vec2 vOtherScale = _pOther->GetScale();

    // 두 사각형이 겹치는지 검사
    if (abs(vPos.x - vOtherPos.x) < (vScale.x + vOtherScale.x) / 2.f &&
        abs(vPos.y - vOtherPos.y) < (vScale.y + vOtherScale.y) / 2.f)
    {
        return true;
    }

    return false;
}

void CCollider::OnCollisionEnter(CCollider* _pOther)
{
    m_pOwner->OnCollisionEnter(_pOther);
}

void CCollider::OnCollision(CCollider* _pOther)
{
    m_pOwner->OnCollision(_pOther);
}

void CCollider::OnCollisionExit(CCollider* _pOther)
{
    m_pOwner->OnCollisionExit(_pOther);
}

void CCollider::RenderScaled(HDC _dc, float _fScale)
{
    // 디버그 모드에서만 충돌체 표시
    if (!KEY_HOLD(KEY::TAB))
        return;

    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetFinalPos());
    Vec2 vScaledSize = m_vScale * _fScale;

    HPEN hRedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hRedPen);
    HBRUSH myBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, myBrush);

    // 스케일이 적용된 충돌체 사각형 그리기
    Rectangle(_dc,
        (int)(vRenderPos.x - vScaledSize.x / 2.f),
        (int)(vRenderPos.y - vScaledSize.y / 2.f),
        (int)(vRenderPos.x + vScaledSize.x / 2.f),
        (int)(vRenderPos.y + vScaledSize.y / 2.f));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hRedPen);
}
