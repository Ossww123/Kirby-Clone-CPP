#include "pch.h"
#include "CSpecialObject.h"
#include "CTimeMgr.h"
#include "CCamera.h"

CSpecialObject::CSpecialObject()
    : m_eSpecialType(OBJECT_TYPE::OBJECT_DOOR)
    , m_bIsActive(true)
    , m_bIsInteractable(true)
    , m_fInteractionRange(80.f)
    , m_fAnimTimer(0.f)
    , m_iAnimFrame(0)
{
    // 기본 오브젝트 타입 설정
    SetType(OBJECT_TYPE::OBJECT_DOOR);
}

CSpecialObject::~CSpecialObject()
{
}

void CSpecialObject::Update()
{
    // 애니메이션 타이머 업데이트
    m_fAnimTimer += CTimeMgr::GetInst()->GetfDT();

    // 기본 애니메이션 처리 (0.1초마다 프레임 변경)
    if (m_fAnimTimer >= 0.1f)
    {
        m_fAnimTimer = 0.f;
        m_iAnimFrame++;

        // 프레임 순환 (4프레임 기준)
        if (m_iAnimFrame >= 4)
            m_iAnimFrame = 0;
    }
}

void CSpecialObject::Render(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 비활성화 상태면 어둡게 표시
    COLORREF color = m_bIsActive ? RGB(255, 255, 255) : RGB(128, 128, 128);

    // 기본 렌더링 (자식 클래스에서 오버라이드)
    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2),
        (int)(vRenderPos.y - vScale.y / 2),
        (int)(vRenderPos.x + vScale.x / 2),
        (int)(vRenderPos.y + vScale.y / 2));

    SelectObject(_dc, hOldBrush);
    SelectObject(_dc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);

    // 상호작용 가능 표시 (디버그용)
    if (m_bIsInteractable && m_bIsActive)
    {
        // 상호작용 범위 표시 (점선 원)
        HPEN hRangePen = CreatePen(PS_DOT, 1, RGB(0, 255, 0));
        HPEN hOldRangePen = (HPEN)SelectObject(_dc, hRangePen);
        HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldRangeBrush = (HBRUSH)SelectObject(_dc, hNullBrush);

        Ellipse(_dc,
            (int)(vRenderPos.x - m_fInteractionRange),
            (int)(vRenderPos.y - m_fInteractionRange),
            (int)(vRenderPos.x + m_fInteractionRange),
            (int)(vRenderPos.y + m_fInteractionRange));

        SelectObject(_dc, hOldRangePen);
        SelectObject(_dc, hOldRangeBrush);
        DeleteObject(hRangePen);
    }
}