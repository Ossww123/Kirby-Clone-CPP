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

#include "CCore.h"

CPlayer::CPlayer()
    : m_pAnimator(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_vVelocity{}
    , m_fSpeed(200.f)
    , m_fJumpPower(400.f)
    , m_bGround(true)
    , m_fGravity(980.f)
{
    // 충돌체 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(80.f, 80.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

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
    Vec2 vPos = GetPos();

    // 좌우 이동
    if (KEY_HOLD(KEY::LEFT))
    {
        m_vVelocity.x = -m_fSpeed;
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        m_vVelocity.x = m_fSpeed;
    }
    else
    {
        m_vVelocity.x = 0.f;
    }

    // 점프
    if (KEY_TAP(KEY::SPACE) && m_bGround)
    {
        m_vVelocity.y = -m_fJumpPower;
        m_bGround = false;
    }

    // 중력 적용
    if (!m_bGround)
    {
        m_vVelocity.y += m_fGravity * CTimeMgr::GetInst()->GetfDT();
    }

    // 위치 업데이트
    vPos.x += m_vVelocity.x * CTimeMgr::GetInst()->GetfDT();
    vPos.y += m_vVelocity.y * CTimeMgr::GetInst()->GetfDT();

    // 간단한 바닥 충돌 (y = 400 기준)
    if (vPos.y >= 400.f)
    {
        vPos.y = 400.f;
        m_vVelocity.y = 0.f;
        m_bGround = true;
    }

    SetPos(vPos);
}

void CPlayer::UpdateState()
{
    PLAYER_STATE eNewState = m_eCurState;

    if (!m_bGround)
    {
        eNewState = PLAYER_STATE::JUMP;
    }
    else if (abs(m_vVelocity.x) > 0.f)
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