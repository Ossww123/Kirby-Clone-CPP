#include "pch.h"
#include "CPlayerStateMachine.h"
#include "CPlayerInhaleSystem.h"
#include "CPlayerMovement.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CAnimation.h"
#include "CEventMgr.h"
#include "CTimeMgr.h"

CPlayerStateMachine::CPlayerStateMachine(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_fFallTime(0.0f)
    , m_fFallToBounceThreshold(1.0f)
    , m_fBounceHeight(0.6f)
    , m_bWasGrounded(true)
    , m_fSlideTimer(0.5f)
{
}

CPlayerStateMachine::~CPlayerStateMachine()
{
}

void CPlayerStateMachine::Init(CAnimator* _pAnimator, CRigidBody* _pRigidBody)
{
    m_pAnimator = _pAnimator;
    m_pRigidBody = _pRigidBody;

    // 초기 상태 설정
    m_pOwner->ChangeState(PLAYER_STATE::IDLE);
}

void CPlayerStateMachine::Update()
{
    if (!m_pOwner || !m_pRigidBody)
        return;

    // 이전 프레임 Ground 상태 저장
    bool currentGrounded = m_pRigidBody->IsGround();

    // 현재 상태에 따른 업데이트
    switch (m_eCurState)
    {
    case PLAYER_STATE::IDLE:
    case PLAYER_STATE::WALK:
    case PLAYER_STATE::RUN:
    case PLAYER_STATE::MOUTHFUL_IDLE:
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
        UpdateMovementState();
        break;

    case PLAYER_STATE::CROUCH:
        UpdateCrouchState();
        break;

    case PLAYER_STATE::SLIDE:
        UpdateSlideState();
        break;

    case PLAYER_STATE::INHALE_READY:
    case PLAYER_STATE::INHALE_1:
    case PLAYER_STATE::INHALE_2:
    case PLAYER_STATE::INHALE_HOLD:
        UpdateInhaleState();
        break;

    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::FALL:
    case PLAYER_STATE::FALL2:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        UpdateAirborneState();
        break;

    case PLAYER_STATE::BOUNCE:
        UpdateBounceState();
        break;

    case PLAYER_STATE::EXHALE:
    case PLAYER_STATE::SWALLOW:
        UpdateSpecialState();
        break;
    }

    // Ground 상태 업데이트
    m_bWasGrounded = currentGrounded;
}

void CPlayerStateMachine::ChangeStateInternal(PLAYER_STATE _eState)
{
    // 유효성 검사
    if (!CanChangeToState(_eState))
        return;

    // 실제 상태 변경
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    // 애니메이션 설정
    SetAnimationForState(_eState);
}

bool CPlayerStateMachine::IsInhaleState() const
{
    return (m_eCurState >= PLAYER_STATE::INHALE_READY &&
        m_eCurState <= PLAYER_STATE::INHALE_HOLD);
}

bool CPlayerStateMachine::IsMovingState() const
{
    return (m_eCurState == PLAYER_STATE::WALK ||
        m_eCurState == PLAYER_STATE::RUN ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_WALK ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_RUN);
}

bool CPlayerStateMachine::IsMouthfulState() const
{
    return (m_eCurState >= PLAYER_STATE::MOUTHFUL_IDLE &&
        m_eCurState <= PLAYER_STATE::MOUTHFUL_JUMP);
}

bool CPlayerStateMachine::IsGroundedState() const
{
    return (m_eCurState != PLAYER_STATE::JUMP &&
        m_eCurState != PLAYER_STATE::FALL &&
        m_eCurState != PLAYER_STATE::FALL2 &&
        m_eCurState != PLAYER_STATE::BOUNCE &&
        m_eCurState != PLAYER_STATE::MOUTHFUL_JUMP);
}

bool CPlayerStateMachine::CanChangeToState(PLAYER_STATE _eState) const
{
    return IsValidStateTransition(m_eCurState, _eState);
}

void CPlayerStateMachine::UpdateMovementState()
{
    if (!m_pRigidBody || !m_pOwner)
        return;

    Vec2 vVelocity = m_pRigidBody->GetVelocity();

    // 공중에 있는 경우 공중 상태로 전환
    if (!m_pRigidBody->IsGround())
    {
        if (vVelocity.y < -50.f)
        {
            // 상승 중
            PLAYER_STATE newState = m_pOwner->HasMouthful() ?
                PLAYER_STATE::MOUTHFUL_JUMP : PLAYER_STATE::JUMP;
            m_pOwner->ChangeState(newState);
        }
        else
        {
            // 하강 중 - FALL 상태로
            m_pOwner->ChangeState(PLAYER_STATE::FALL);
            m_fFallTime = 0.0f;  // 낙하 시간 초기화
        }
        return;
    }

    // 기존 땅에서의 이동 로직...
    CPlayerMovement* pMovement = m_pOwner->GetMovement();
    if (!pMovement)
        return;

    bool isActuallyMoving = pMovement->IsActuallyMoving();
    bool isDecelerating = pMovement->IsDecelerating();
    bool hasMouthful = m_pOwner->HasMouthful();
    bool isRunMode = pMovement->IsRunMode();

    if (isActuallyMoving || isDecelerating)
    {
        // 실제로 움직이고 있거나 감속 중이면 이동 상태
        if (hasMouthful)
        {
            PLAYER_STATE newState = isRunMode ?
                PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::MOUTHFUL_WALK;
            m_pOwner->ChangeState(newState);
        }
        else
        {
            PLAYER_STATE newState = isRunMode ?
                PLAYER_STATE::RUN : PLAYER_STATE::WALK;
            m_pOwner->ChangeState(newState);
        }
    }
    else
    {
        // 정지 상태
        PLAYER_STATE newState = hasMouthful ?
            PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
        m_pOwner->ChangeState(newState);
    }
}

void CPlayerStateMachine::UpdateInhaleState()
{
    if (!m_pOwner)
        return;

    CPlayerInhaleSystem* pInhaleSystem = m_pOwner->GetInhaleSystem();
    if (!pInhaleSystem)
        return;

    float inhaleTime = pInhaleSystem->GetInhaleTime();

    if (!m_pOwner->IsInhaling())
    {
        // 흡입이 끝났으면 상태 변경
        const vector<CObject*>& vecTargets = pInhaleSystem->GetInhaleTargets();
        if (!vecTargets.empty())
        {
            m_pOwner->ChangeState(PLAYER_STATE::SWALLOW);
        }
        else
        {
            m_pOwner->ChangeState(PLAYER_STATE::EXHALE);
        }
        return;
    }

    // 흡입 시간에 따른 상태 변경
    PLAYER_STATE newState = m_eCurState;

    if (inhaleTime < 0.5f)
    {
        newState = PLAYER_STATE::INHALE_1;
    }
    else if (inhaleTime < 1.0f)
    {
        newState = PLAYER_STATE::INHALE_2;
    }
    else
    {
        newState = PLAYER_STATE::INHALE_HOLD;
    }

    if (newState != m_eCurState)
    {
        m_pOwner->ChangeState(newState);
    }
}

// === 공중 상태 업데이트 ===
void CPlayerStateMachine::UpdateAirborneState()
{
    if (!m_pRigidBody)
        return;

    Vec2 vVelocity = m_pRigidBody->GetVelocity();
    bool isGrounded = m_pRigidBody->IsGround();

    // 착지 처리
    if (!m_bWasGrounded && isGrounded)
    {
        HandleLanding();
        return;
    }

    // 공중에 있을 때의 상태 전환
    if (!isGrounded)
    {
        if (m_eCurState == PLAYER_STATE::JUMP)
        {
            // 상승 중에서 하강으로 전환
            if (vVelocity.y >= 0.f)
            {
                m_pOwner->ChangeState(PLAYER_STATE::FALL);
                m_fFallTime = 0.0f;  // 낙하 시간 초기화
            }
        }
        else if (m_eCurState == PLAYER_STATE::FALL)
        {
            // 낙하 시간 누적
            m_fFallTime += CTimeMgr::GetInst()->GetfDT();

            // 일정 시간 낙하하면 FALL2로 전환
            if (m_fFallTime >= m_fFallToBounceThreshold)
            {
                m_pOwner->ChangeState(PLAYER_STATE::FALL2);
            }
        }
        // FALL2 상태에서는 계속 낙하만 함
    }
}

void CPlayerStateMachine::UpdateSpecialState()
{
    if (!m_pAnimator)
        return;

    CAnimation* pCurAnim = m_pAnimator->GetCurAnim();
    if (!pCurAnim)
        return;

    // 애니메이션이 끝났는지 체크
    if (pCurAnim->IsFinish())
    {
        switch (m_eCurState)
        {
        case PLAYER_STATE::EXHALE:
            m_pOwner->ChangeState(PLAYER_STATE::IDLE);
            break;

        case PLAYER_STATE::SWALLOW:
            // 삼키기 완료 후 입에 물고 있는 상태로
            if (m_pOwner)
            {
                if (m_pOwner->GetInhaleSystem())
                    m_pOwner->GetInhaleSystem()->SetMouthful(false);
                m_pOwner->ChangeState(PLAYER_STATE::MOUTHFUL_IDLE);
            }
            break;
        }
    }
}

// === 바운스 상태 업데이트 ===
void CPlayerStateMachine::UpdateBounceState()
{
    if (!m_pRigidBody)
    {
        return;
    }

    Vec2 vVelocity = m_pRigidBody->GetVelocity();
    bool isGrounded = m_pRigidBody->IsGround();

    if (!isGrounded && vVelocity.y >= 0.f)
    {
        m_pOwner->ChangeState(PLAYER_STATE::FALL);
        m_fFallTime = 0.0f;
    }
}

// === 크라우치 상태 업데이트 ===
void CPlayerStateMachine::UpdateCrouchState()
{
    if (!m_pOwner || !m_pRigidBody)
        return;

    // 공중에 있으면 낙하로 전환
    if (!m_pRigidBody->IsGround())
    {
        m_pOwner->ChangeState(PLAYER_STATE::FALL);
        m_fFallTime = 0.0f;
        return;
    }

    // 크라우치 입력이 해제되면 IDLE로 복귀 (MovementSystem에서 처리하지만 이중 체크)
    CPlayerMovement* pMovement = m_pOwner->GetMovement();
    if (pMovement && !pMovement->IsCrouchInputPressed())
    {
        m_pOwner->ChangeState(PLAYER_STATE::IDLE);
        return;
    }

    // 크라우치 상태에서는 특별한 상태 전환 없음 (입력은 Movement에서 처리)
}

// === 슬라이드 상태 업데이트 ===
void CPlayerStateMachine::UpdateSlideState()
{
    if (!m_pOwner || !m_pRigidBody)
        return;

    // 슬라이드 중 공중에 떨어지면 낙하로 전환
    if (!m_pRigidBody->IsGround())
    {
        m_pOwner->ChangeState(PLAYER_STATE::FALL);
        m_fFallTime = 0.0f;
        return;
    }

    // 슬라이드 애니메이션이 끝났는지 체크
    CAnimator* pAnimator = m_pOwner->GetAnimator();
    if (pAnimator)
    {
        CAnimation* pCurAnim = pAnimator->GetCurAnim();
        if (pCurAnim && pCurAnim->IsFinish())
        {
            // 슬라이드 완료 후 처리
            CPlayerMovement* pMovement = m_pOwner->GetMovement();
            if (pMovement && pMovement->IsCrouchInputPressed())
            {
                // DOWN 키가 여전히 눌려있으면 크라우치로 복귀
                m_pOwner->ChangeState(PLAYER_STATE::CROUCH);
            }
            else
            {
                // DOWN 키가 해제되어있으면 IDLE로
                m_pOwner->ChangeState(PLAYER_STATE::IDLE);
            }
        }
    }

    // 애니메이션이 없거나 문제가 있으면 일정 시간 후 자동 종료
    if (m_fSlideTimer >= 0.5f)  // 0.5초 후 강제 종료
    {
        CPlayerMovement* pMovement = m_pOwner->GetMovement();
        if (pMovement && pMovement->IsCrouchInputPressed())
        {
            m_pOwner->ChangeState(PLAYER_STATE::CROUCH);
        }
        else
        {
            m_pOwner->ChangeState(PLAYER_STATE::IDLE);
        }
    }
}

// === 착지 처리 ===
void CPlayerStateMachine::HandleLanding()
{
    if (!m_pOwner)
        return;

    // 이전 상태에 따라 착지 처리
    switch (m_eCurState)
    {
    case PLAYER_STATE::FALL:
        // 일반 낙하에서 착지 - 바로 IDLE
    {
        bool hasMouthful = m_pOwner->HasMouthful();
        PLAYER_STATE newState = hasMouthful ?
            PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
        m_pOwner->ChangeState(newState);
    }
    break;

    case PLAYER_STATE::FALL2:
        // 장시간 낙하에서 착지 - BOUNCE
        m_pOwner->ChangeState(PLAYER_STATE::BOUNCE);
        PerformBounce();
        break;

    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        // 점프 상태에서 직접 착지 (매우 낮은 점프)
    {
        bool hasMouthful = m_pOwner->HasMouthful();
        PLAYER_STATE newState = hasMouthful ?
            PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
        m_pOwner->ChangeState(newState);
    }
    break;
    }

    // 낙하 시간 초기화
    m_fFallTime = 0.0f;
}

// === 바운스 실행 ===
void CPlayerStateMachine::PerformBounce()
{
    if (!m_pRigidBody || !m_pOwner)
        return;

    // 플레이어의 점프력 가져오기
    CPlayerMovement* pMovement = m_pOwner->GetMovement();
    if (pMovement)
    {
        float baseJumpPower = pMovement->GetJumpPower();
        float bounceJumpPower = baseJumpPower * m_fBounceHeight;

        // 바운스 점프 실행
        m_pRigidBody->SetVelocityY(-bounceJumpPower);
        m_pRigidBody->SetGround(false);
    }
}

void CPlayerStateMachine::SetAnimationForState(PLAYER_STATE _eState)
{
    if (!m_pAnimator)
    {
        return;
    }

    // 현재 재생 중인 애니메이션 확인
    CAnimation* pCurAnim = m_pAnimator->GetCurAnim();
    if (pCurAnim)
    {
        char buffer[256];
        sprintf_s(buffer, "Current animation exists, switching to state: %d\n", (int)_eState);
        OutputDebugStringA(buffer);
    }

    // 상태에 맞는 애니메이션 재생
    switch (_eState)
    {
    case PLAYER_STATE::IDLE:
        m_pAnimator->Play(L"IDLE", true);
        break;
    case PLAYER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case PLAYER_STATE::RUN:
        m_pAnimator->Play(L"RUN", true);
        break;
    case PLAYER_STATE::CROUCH:
        m_pAnimator->Play(L"CROUCH", true);
        break;
    case PLAYER_STATE::SLIDE:
        m_pAnimator->Play(L"SLIDE", true);
        break;
    case PLAYER_STATE::JUMP:
        m_pAnimator->Play(L"JUMP", false);
        break;
    case PLAYER_STATE::FALL:
        m_pAnimator->Play(L"FALL", true);
        break;
    case PLAYER_STATE::FALL2:
        m_pAnimator->Play(L"FALL2", true);
        break;
    case PLAYER_STATE::BOUNCE:
        m_pAnimator->Play(L"BOUNCE", false);
        break;
    case PLAYER_STATE::INHALE_READY:
        m_pAnimator->Play(L"INHALE_READY", false);
        break;
    case PLAYER_STATE::INHALE_1:
        m_pAnimator->Play(L"INHALE_1", true);
        break;
    case PLAYER_STATE::INHALE_2:
        m_pAnimator->Play(L"INHALE_2", true);
        break;
    case PLAYER_STATE::INHALE_HOLD:
        m_pAnimator->Play(L"INHALE_HOLD", true);
        break;
    case PLAYER_STATE::EXHALE:
        m_pAnimator->Play(L"EXHALE", false);
        break;
    case PLAYER_STATE::SWALLOW:
        m_pAnimator->Play(L"SWALLOW", false);
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        m_pAnimator->Play(L"MOUTHFUL_IDLE", true);
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
        m_pAnimator->Play(L"MOUTHFUL_WALK", true);
        break;
    case PLAYER_STATE::MOUTHFUL_RUN:
        m_pAnimator->Play(L"MOUTHFUL_RUN", true);
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        m_pAnimator->Play(L"MOUTHFUL_JUMP", false);
        break;
    default:
        char buffer[256];
        sprintf_s(buffer, "WARNING: No animation case for state: %d\n", (int)_eState);
        OutputDebugStringA(buffer);
        break;
    }
}

bool CPlayerStateMachine::IsValidStateTransition(PLAYER_STATE _from, PLAYER_STATE _to) const
{
    // 같은 상태로의 전환은 허용하지 않음
    if (_from == _to)
        return false;

    // 특정 상태에서는 특정 상태로만 전환 가능
    switch (_from)
    {
    case PLAYER_STATE::SWALLOW:
        // 삼키기 중에는 다른 상태로 전환 불가 (애니메이션 완료 후에만)
        return false;

    case PLAYER_STATE::EXHALE:
        // 내뱉기 중에는 다른 상태로 전환 불가 (애니메이션 완료 후에만)
        return false;
    }

    // 기본적으로 모든 전환 허용
    return true;
}