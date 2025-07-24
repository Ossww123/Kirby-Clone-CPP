#include "pch.h"
#include "CCollider.h"
#include "CObject.h"
#include "CCamera.h"

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