#include "gamePCH.h"
#include "CCollider.h"
#include "CObject.h"
#include "CCamera.h"
#include "CKeyMgr.h"

UINT CCollider::g_iNextID = 0;

// === 생성자 & 소멸자 ===
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

// === 최종 업데이트 함수들 ===
void CCollider::FinalUpdate()
{
    // 충돌체의 위치 계산
}

void CCollider::Render(HDC _dc)
{
    return; // 충돌체 렌더링 비활성화
    // 월드 좌표를 카메라 좌표로 변환
    Vec2 vPos = GetFinalPos();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 충돌체 시각화 (속이 비어있는 사각형)
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 충돌체 선
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 사각형 그리기
    Rectangle(_dc, int(vRenderPos.x - m_vScale.x / 2.f)
        , int(vRenderPos.y - m_vScale.y / 2.f)
        , int(vRenderPos.x + m_vScale.x / 2.f)
        , int(vRenderPos.y + m_vScale.y / 2.f));

    // 리소스 정리
    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CCollider::RenderScaled(HDC _dc, float _fScale)
{
    // 에디터 모드에서의 충돌체 표시
    if (!KEY_HOLD(KEY::TAB))
        return;

    // 렌더링 위치 계산
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetFinalPos());
    Vec2 vScaledSize = m_vScale * _fScale;

    // 에디터 전용 색상
    HPEN hRedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hRedPen);
    HBRUSH myBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, myBrush);

    // 스케일링이 적용된 충돌체 사각형 그리기
    Rectangle(_dc,
        (int)(vRenderPos.x - vScaledSize.x / 2.f),
        (int)(vRenderPos.y - vScaledSize.y / 2.f),
        (int)(vRenderPos.x + vScaledSize.x / 2.f),
        (int)(vRenderPos.y + vScaledSize.y / 2.f));

    // 리소스 정리
    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hRedPen);
}

// === 충돌 검사 ===
bool CCollider::IsCollision(CCollider* _pOther)
{
    // 위치 정보 계산
    Vec2 vPos = GetFinalPos();
    Vec2 vOtherPos = _pOther->GetFinalPos();

    // 크기 정보 계산
    Vec2 vScale = GetScale();
    Vec2 vOtherScale = _pOther->GetScale();

    // AABB 충돌 검사 - 두 사각형의 겹침여부 검사
    if (abs(vPos.x - vOtherPos.x) < (vScale.x + vOtherScale.x) / 2.f &&
        abs(vPos.y - vOtherPos.y) < (vScale.y + vOtherScale.y) / 2.f)
    {
        return true;
    }

    return false;
}

// === 충돌 이벤트 처리 ===
void CCollider::OnCollisionEnter(CCollider* _pOther)
{
    // 소유자에게 충돌 시작 이벤트 전달
    m_pOwner->OnCollisionEnter(_pOther);
}

void CCollider::OnCollision(CCollider* _pOther)
{
    // 소유자에게 충돌 중 이벤트 전달
    m_pOwner->OnCollision(_pOther);
}

void CCollider::OnCollisionExit(CCollider* _pOther)
{
    // 소유자에게 충돌 종료 이벤트 전달
    m_pOwner->OnCollisionExit(_pOther);
}

// === 위치 및 크기 계산 ===
Vec2 CCollider::GetFinalPos()
{
    // 오브젝트 위치에 오프셋 더하기
    Vec2 vObjectPos = m_pOwner->GetPos();
    return vObjectPos + m_vOffsetPos;
}

// === 충돌 목록 관리 ===
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
    // 목록에서 해당 콜라이더 제거
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