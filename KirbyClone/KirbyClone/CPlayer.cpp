#include "pch.h"
#include "CPlayer.h"

#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CCollider.h"
#include "CEventMgr.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CAnimator.h"
#include "CRigidBody.h"

#include "CCore.h"

CPlayer::CPlayer()
    : m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_fSpeed(200.f)
    , m_fJumpPower(400.f)
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(80.f, 80.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

    // 리지드바디 생성
    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정
    m_pRigidBody->SetMass(1.f);
    m_pRigidBody->SetMaxVelocity(500.f);
    m_pRigidBody->SetFriction(10.f);
    m_pRigidBody->SetUseGravity(true);

    // 애니메이션 생성
    CreateAnimation();

    // 초기 애니메이션 재생
    m_pAnimator->Play(L"IDLE", true);
}

CPlayer::~CPlayer()
{
    // 부모 클래스에서 이미 m_pAnimator를 삭제하므로
    // 여기서는 nullptr로만 설정
    m_pAnimator = nullptr;
    m_pRigidBody = nullptr;
}

void CPlayer::CreateAnimation()
{
    // 커비 스프라이트 시트 로드
    CTexture* pTex = CResMgr::GetInst()->LoadTexture(L"KirbySprite", L"texture\\kirby_sprite.bmp");

    // IDLE 애니메이션 (실제 스프라이트에 맞게 조정 필요)
    m_pAnimator->CreateAnimation(L"IDLE", pTex, Vec2(0, 0), Vec2(32, 32), Vec2(32, 0), 0.3f, 4, true);

    // WALK 애니메이션 (실제 스프라이트에 맞게 조정 필요)
    m_pAnimator->CreateAnimation(L"WALK", pTex, Vec2(0, 32), Vec2(32, 32), Vec2(32, 0), 0.15f, 8, true);

    // JUMP 애니메이션 (실제 스프라이트에 맞게 조정 필요)
    m_pAnimator->CreateAnimation(L"JUMP", pTex, Vec2(0, 64), Vec2(32, 32), Vec2(32, 0), 0.1f, 1, false);
}

void CPlayer::Update()
{
    UpdateMove();
    UpdateState();

    // 리지드바디 업데이트
    if (nullptr != m_pRigidBody)
        m_pRigidBody->Update();

    // 애니메이터 업데이트
    if (nullptr != m_pAnimator)
        m_pAnimator->Update();

    // C키로 카메라가 플레이어를 따라가도록 설정/해제
    if (KEY_TAP(KEY::C))
    {
        if (CCamera::GetInst()->GetTarget() == this)
        {
            CCamera::GetInst()->SetTarget(nullptr);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 해제");
        }
        else
        {
            CCamera::GetInst()->SetTarget(this);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"카메라 추적 시작");
        }
    }
}

void CPlayer::UpdateMove()
{
    if (nullptr == m_pRigidBody)
        return;

    // 점프 (바닥에 있을 때만)
    if (KEY_TAP(KEY::SPACE) && m_pRigidBody->IsGround())
    {
        // 점프 직전에 중력 영향을 받지 않도록 즉시 바닥 상태 해제
        m_pRigidBody->SetGround(false);
        m_pRigidBody->SetVelocityY(-m_fJumpPower);
    }

    // 좌우 이동 (리지드바디의 X 속도 직접 설정)
    if (KEY_HOLD(KEY::LEFT))
    {
        m_pRigidBody->SetVelocityX(-m_fSpeed);
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        m_pRigidBody->SetVelocityX(m_fSpeed);
    }
    else
    {
        // 키를 누르지 않으면 X축 속도를 0으로 (마찰로 자연스럽게 감속)
        if (m_pRigidBody->IsGround())
        {
            m_pRigidBody->SetVelocityX(0.f);
        }
    }

    // 간단한 바닥 충돌 처리 (y = 400 기준)
    Vec2 vPos = GetPos();
    Vec2 vVelocity = m_pRigidBody->GetVelocity();

    if (vPos.y >= 400.f && vVelocity.y >= 0.f)
    {
        vPos.y = 400.f;
        SetPos(vPos);
        m_pRigidBody->SetVelocityY(0.f);
        m_pRigidBody->SetGround(true);
    }
}

void CPlayer::UpdateState()
{
    if (nullptr == m_pRigidBody)
        return;

    PLAYER_STATE eNewState = m_eCurState;
    Vec2 vVelocity = m_pRigidBody->GetVelocity();

    if (!m_pRigidBody->IsGround())
    {
        eNewState = PLAYER_STATE::JUMP;
    }
    else if (abs(vVelocity.x) > 10.f)  // 속도가 일정 이상일 때만 걷기 상태
    {
        eNewState = PLAYER_STATE::WALK;
    }
    else
    {
        eNewState = PLAYER_STATE::IDLE;
    }

    // 상태가 변경되었다면 애니메이션 변경
    if (m_eCurState != eNewState)
    {
        ChangeState(eNewState);
    }
}


void CPlayer::ChangeState(PLAYER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    // 상태에 맞는 애니메이션 재생
    switch (m_eCurState)
    {
    case PLAYER_STATE::IDLE:
        m_pAnimator->Play(L"IDLE", true);
        break;
    case PLAYER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case PLAYER_STATE::JUMP:
        m_pAnimator->Play(L"JUMP", false);
        break;
    }
}

void CPlayer::Render(HDC _dc)
{
    // 애니메이션 렌더링
    if (nullptr != m_pAnimator)
    {
        m_pAnimator->Render(_dc);
    }

    // 충돌체가 있으면 충돌체도 렌더링
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
}



void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();

    // 충돌 시 화면 흔들림 효과
    CCamera::GetInst()->CameraShake(0.3f, 10.f);

    // 디버그 출력 (윈도우 타이틀에 표시)
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"충돌 시작!");

    // 필요하다면 여기서 추가 이벤트
    // 예: 파티클 이펙트, 사운드 재생, UI 업데이트 등
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리 (매 프레임 호출)
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"충돌 끝!");
}