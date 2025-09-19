#include "pch.h"
#include "CDoor.h"

#include "CCollider.h"
#include "CKeyMgr.h"
#include "CEventMgr.h"
#include "CTimeMgr.h"
#include "CCamera.h"
#include "CCore.h"
#include "CPlayerDataMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CAnimationDataMgr.h"
#include "CResMgr.h"
#include "CPlayerStateMachine.h"
#include "CSoundMgr.h"

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

    // 애니메이터 생성 및 애니메이션 로드
    CreateAnimator();
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        // door_animations.json 로드 (CMonster와 같은 방식)
        CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, L"door_animations.json");
        
        // 기본 애니메이션 설정 (IDLE 상태로 반복 재생)
        pAnimator->Play(L"IDLE", true);
    }
}

CDoor::~CDoor()
{
}

void CDoor::Update()
{
    // 애니메이터 업데이트 (애니메이션 재생을 위해 필수!)
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Update();
    }
    // 플레이어와의 상호작용 체크
    CheckPlayerInteraction();

    // 상호작용 가능한 상태에서 키 입력 체크
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

    // 콜라이더 렌더링 추가 (녹색 사각형)
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
    // 현재 플레이어 상태 저장
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurrentScene)
    {
        // 플레이어 찾기
        const vector<CObject*>& vecPlayer = pCurrentScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayer.empty() && vecPlayer[0])
        {
            CPlayer* pPlayer = dynamic_cast<CPlayer*>(vecPlayer[0]);
            if (pPlayer)
            {
                // 플레이어에게 DOOR_ENTER 상태 강제 변경
                pPlayer->GetStateMachine()->ForceStateChange(PLAYER_STATE::DOOR_ENTER);
                
                // 플레이어 상태를 매니저에 저장 (능력 등)
                CPlayerDataMgr::GetInst()->SavePlayerState(pPlayer);
            }
        }
    }

    // 문 입장 이벤트 발생 (페이드 아웃과 씬 변경을 위해)
    tEvent doorEvent(EVENT_TYPE::DOOR_ENTER, (DWORD_PTR)this, (DWORD_PTR)m_eTargetScene);
    CEventMgr::GetInst()->AddEvent(doorEvent);

    // 문 입장 효과음
    CSoundMgr::GetInst()->PlaySFX(L"enter_door");
}

void CDoor::RenderDoorVisual(HDC _dc)
{
    // 애니메이션 렌더링
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Render(_dc);
        return;
    }

    // 애니메이션이 없는 경우 기본 렌더링 (폴백)
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 기본 문 모양 그리기 (임시 - 벡터 영상 지형지물로 교체)
    HBRUSH hBrush;
    HBRUSH hOldBrush;

    if (m_bCanInteract)
    {
        // 상호작용 가능한 문 색상 표시
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