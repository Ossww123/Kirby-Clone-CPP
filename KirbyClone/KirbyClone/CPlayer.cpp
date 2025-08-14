#include "pch.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CRigidBody.h"
#include "CCollider.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"
#include "CTile.h"
#include "CMonster.h"
#include "CAnimationDataMgr.h"
#include "CResMgr.h"
#include "CCore.h"
#include "CCamera.h"

#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayerHealthSystem.h"
#include "CPlayerCollisionSystem.h"

CPlayer::CPlayer()
    : m_pStateMachine(nullptr)
    , m_pInhaleSystem(nullptr)
    , m_pMovement(nullptr)
    , m_pHealthSystem(nullptr)
    , m_pCollisionSystem(nullptr)
{
    SetType(OBJECT_TYPE::PLAYER);

    // === 컴포넌트들 생성 (기존 방식) ===
    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 설정 (올바른 함수명 사용)
    CRigidBody* pRigidBody = GetRigidBody();
    pRigidBody->SetMass(1.f);
    pRigidBody->SetMaxVelocity(500.f);
    pRigidBody->SetFriction(0.1f);
    pRigidBody->SetUseGravity(true);

    // 콜라이더 생성 및 설정
    CreateCollider();
    GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // === 시스템들 생성 ===
    m_pStateMachine = new CPlayerStateMachine(this);
    m_pInhaleSystem = new CPlayerInhaleSystem(this);
    m_pMovement = new CPlayerMovement(this);
    m_pHealthSystem = new CPlayerHealthSystem(this);
    m_pCollisionSystem = new CPlayerCollisionSystem(this);

    // === 시스템들 초기화 ===
    m_pStateMachine->Init(GetAnimator(), GetRigidBody());
    m_pInhaleSystem->Init();
    m_pMovement->Init(GetRigidBody());
    m_pHealthSystem->Init();

    // === 애니메이션 생성 ===
    CreateAnimation();

    // === 기본 설정 ===
    SetPos(Vec2(640.f, 384.f));
    SetScale(Vec2(64.f, 64.f));
}

CPlayer::~CPlayer()
{
    if (m_pStateMachine)
    {
        delete m_pStateMachine;
        m_pStateMachine = nullptr;
    }

    if (m_pInhaleSystem)
    {
        delete m_pInhaleSystem;
        m_pInhaleSystem = nullptr;
    }

    if (m_pMovement)
    {
        delete m_pMovement;
        m_pMovement = nullptr;
    }

    if (m_pHealthSystem)
    {
        delete m_pHealthSystem;
        m_pHealthSystem = nullptr;
    }

    if (m_pCollisionSystem)
    {
        delete m_pCollisionSystem;
        m_pCollisionSystem = nullptr;
    }
}

void CPlayer::Update()
{
    // === 체력 시스템 업데이트 ===
    if (m_pHealthSystem)
        m_pHealthSystem->Update();

    // 게임오버 상태라면 다른 업데이트 중지
    if (m_pHealthSystem && m_pHealthSystem->IsGameOver())
        return;

    // === 흡입 관련 입력 처리 ===
    UpdateInhale();

    // === 이동 시스템 업데이트 ===
    if (m_pMovement)
        m_pMovement->Update();

    // === 흡입 시스템 업데이트 ===
    if (m_pInhaleSystem)
        m_pInhaleSystem->Update();

    // === 리지드바디 업데이트 ===
    CRigidBody* pRigidBody = GetRigidBody();
    if (pRigidBody)
        pRigidBody->Update();

    // === 상태 머신 업데이트 ===
    if (m_pStateMachine)
        m_pStateMachine->Update();

    // === 애니메이터 업데이트 ===
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
        pAnimator->Update();
}

void CPlayer::Render(HDC _dc)
{
    // 무적 상태에서는 깜빡거리는 렌더링
    if (m_pHealthSystem && m_pHealthSystem->ShouldRenderBlink())
    {
        RenderInvincible(_dc);
        return;
    }

    // 기본 렌더링
    CAnimator* pAnimator = GetAnimator();
    if (pAnimator)
    {
        pAnimator->Render(_dc);
    }
    else
    {
        // 애니메이터가 없으면 기본 오브젝트 렌더링
        CObject::Render(_dc);
    }

    // 충돌체 렌더링 추가
    if (GetCollider())
    {
        //GetCollider()->Render(_dc);  // 항상 보이는 초록색
        // 또는
        GetCollider()->RenderScaled(_dc, 1.0f);  // TAB키 필요
    }

    // 흡입 시스템 이펙트 렌더링
    if (m_pInhaleSystem)
    {
        m_pInhaleSystem->RenderInhaleEffect(_dc);
    }
}

void CPlayer::OnCollisionEnter(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollisionEnter(_pOther);
}

void CPlayer::OnCollision(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollision(_pOther);
}

void CPlayer::OnCollisionExit(CCollider* _pOther)
{
    if (m_pCollisionSystem)
        m_pCollisionSystem->HandleCollisionExit(_pOther);
}

// === 상태 관련 인터페이스 구현 ===
PLAYER_STATE CPlayer::GetCurrentState() const
{
    return m_pStateMachine ? m_pStateMachine->GetCurrentState() : PLAYER_STATE::IDLE;
}

PLAYER_STATE CPlayer::GetPreviousState() const
{
    return m_pStateMachine ? m_pStateMachine->GetPreviousState() : PLAYER_STATE::END;
}

void CPlayer::ChangeState(PLAYER_STATE _eState)
{
    if (m_pStateMachine)
        m_pStateMachine->ChangeState(_eState);
}

// === 필수 래퍼 함수들 구현 ===

// 흡입 관련 필수 기능
bool CPlayer::IsInhaling() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->IsInhaling() : false;
}

bool CPlayer::HasMouthful() const
{
    return m_pInhaleSystem ? m_pInhaleSystem->HasMouthful() : false;
}

void CPlayer::StartInhale()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->StartInhale();
}

void CPlayer::StopInhale()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->StopInhale();
}

void CPlayer::SpitOut()
{
    if (m_pInhaleSystem)
        m_pInhaleSystem->SpitOut();
}

// 이동 관련 필수 기능
bool CPlayer::IsFacingRight() const
{
    return m_pMovement ? m_pMovement->IsFacingRight() : true;
}

// 체력 관련 필수 기능
void CPlayer::TakeDamage(int _iDamage, Vec2 _vKnockbackDir)
{
    if (m_pHealthSystem)
        m_pHealthSystem->TakeDamage(_iDamage, _vKnockbackDir);
}

bool CPlayer::IsGameOver() const
{
    return m_pHealthSystem ? m_pHealthSystem->IsGameOver() : false;
}

bool CPlayer::ShouldRenderBlink() const
{
    return m_pHealthSystem ? m_pHealthSystem->ShouldRenderBlink() : false;
}

// === 렌더링 헬퍼 함수들 ===
void CPlayer::RenderInvincible(HDC _dc)
{
    // 무적 상태 확인
    bool bShouldRender = true;
    if (m_pHealthSystem && m_pHealthSystem->ShouldRenderBlink())
    {
        // 깜빡거리는 효과 - 일정 시간마다 렌더링 건너뛰기
        static float fBlinkTime = 0.f;
        fBlinkTime += CTimeMgr::GetInst()->GetfDT();

        if (fBlinkTime > 0.1f)  // 0.1초마다 토글
        {
            bShouldRender = !bShouldRender;
            fBlinkTime = 0.f;
        }
    }

    if (bShouldRender)
    {
        // 기본 렌더링
        CAnimator* pAnimator = GetAnimator();
        if (pAnimator)
        {
            pAnimator->Render(_dc);
        }
        else
        {
            CObject::Render(_dc);
        }
    }
}

// === 업데이트 헬퍼 함수들 ===
void CPlayer::UpdateInhale()
{
    // 흡입 입력 처리
    if (KEY_HOLD(KEY::X))
    {
        if (!IsInhaling())
        {
            StartInhale();
        }
    }
    else
    {
        if (IsInhaling())
        {
            StopInhale();
        }
    }

    // 뱉기 입력 처리
    if (KEY_TAP(KEY::Z))
    {
        if (HasMouthful())
        {
            SpitOut();
        }
    }

    // 삼키기 입력 처리 (직접 시스템 접근)
    if (KEY_TAP(KEY::C))
    {
        if (m_pInhaleSystem && m_pInhaleSystem->HasMouthful())
        {
            CObject* pTarget = m_pInhaleSystem->GetMouthfulTarget();
            if (pTarget)
            {
                m_pInhaleSystem->SwallowTarget(pTarget);
            }
        }
    }
}

// === 애니메이션 생성 함수 ===
void CPlayer::CreateAnimation()
{
    // 애니메이션 매니저를 통한 파일 기반 로딩
    CAnimator* pAnimator = GetAnimator();
    if (!pAnimator)
        return;

    // 플레이어 애니메이션 파일 로드
    wstring animationFilePath = L"player_animations.json";
    CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(pAnimator, animationFilePath);

    // 기본 애니메이션 설정
    pAnimator->Play(L"IDLE", true);
}