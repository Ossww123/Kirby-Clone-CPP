#include "pch.h"
#include "CDoor.h"

#include "CCollider.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"
#include "CTimeMgr.h"
#include "CCamera.h"
#include "CCore.h"

CDoor::CDoor()
    : CSpecialObject()
    , m_eTargetScene(SCENE_TYPE::STAGE_01)
    , m_vTargetPosition(Vec2(256.f, 384.f))
    , m_bPlayerNear(false)
    , m_bCanInteract(false)
{
    // 문 타입으로 설정
    SetSpecialType(OBJECT_TYPE::OBJECT_DOOR);
    SetType(OBJECT_TYPE::OBJECT_DOOR);

    // 기본 문 크기 설정
    SetScale(Vec2(64.f, 64.f));

    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(64.f, 64.f));
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
}

CDoor::~CDoor()
{
}

void CDoor::Update()
{
    // 플레이어와의 상호작용 체크
    CheckPlayerInteraction();

    // 상호작용 가능할 때 키 입력 체크
    if (m_bCanInteract)
    {
        if (KEY_TAP(KEY::UP) || KEY_TAP(KEY::W))
        {
            ProcessDoorTransition();
        }
    }
}

void CDoor::Render(HDC _dc)
{
    // 문 시각적 표현 렌더링
    RenderDoorVisual(_dc);

    // 콜라이더 렌더링 추가 (초록색 사각형)
    if (GetCollider())
    {
        GetCollider()->Render(_dc);
    }

    // 상호작용 UI 렌더링
    if (m_bCanInteract)
    {
        RenderInteractionUI(_dc);
    }
}

void CDoor::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pObj = _pOther->GetOwner();

    // 플레이어인지 확인
    if (pObj && pObj->GetType() == OBJECT_TYPE::PLAYER)
    {
        m_bPlayerNear = true;
    }
}

void CDoor::OnCollisionExit(CCollider* _pOther)
{
    CObject* pObj = _pOther->GetOwner();

    // 플레이어인지 확인
    if (pObj && pObj->GetType() == OBJECT_TYPE::PLAYER)
    {
        m_bPlayerNear = false;
        m_bCanInteract = false;
    }
}

void CDoor::CheckPlayerInteraction()
{
    m_bCanInteract = m_bPlayerNear;
}

void CDoor::ProcessDoorTransition()
{
    // 씬 전환 이벤트 발생
    tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)m_eTargetScene);
    CEventMgr::GetInst()->AddEvent(event);

    // TODO: 플레이어 위치 설정을 위한 추가 작업 필요
    // 현재는 각 씬의 Enter()에서 기본 위치로 설정됨

    // 문 사용 사운드 재생 (추후 추가)
    // CSoundMgr::GetInst()->PlaySFX(L"door_open");
}

void CDoor::RenderDoorVisual(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 기본 문 모양 그리기 (임시 - 추후 스프라이트로 교체)
    HBRUSH hBrush;
    HBRUSH hOldBrush;

    if (m_bCanInteract)
    {
        // 상호작용 가능할 때 밝게 표시
        hBrush = CreateSolidBrush(RGB(160, 82, 45)); // 밝은 갈색
    }
    else
    {
        hBrush = CreateSolidBrush(RGB(101, 67, 33)); // 기본 갈색
    }

    hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2),
        (int)(vRenderPos.y - vScale.y / 2),
        (int)(vRenderPos.x + vScale.x / 2),
        (int)(vRenderPos.y + vScale.y / 2));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 문 손잡이 그리기
    HBRUSH hKnobBrush = CreateSolidBrush(RGB(255, 215, 0)); // 금색
    hOldBrush = (HBRUSH)SelectObject(_dc, hKnobBrush);

    Ellipse(_dc,
        (int)(vRenderPos.x + vScale.x / 4 - 8),
        (int)(vRenderPos.y - 8),
        (int)(vRenderPos.x + vScale.x / 4 + 8),
        (int)(vRenderPos.y + 8));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hKnobBrush);
}

void CDoor::RenderInteractionUI(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());

    // 상호작용 안내 텍스트
    SetTextColor(_dc, RGB(255, 255, 0));
    SetBkMode(_dc, TRANSPARENT);

    RECT textRect;
    textRect.left = (int)(vRenderPos.x - 50);
    textRect.top = (int)(vRenderPos.y - GetScale().y / 2 - 50);
    textRect.right = (int)(vRenderPos.x + 50);
    textRect.bottom = (int)(vRenderPos.y - GetScale().y / 2 - 30);
}