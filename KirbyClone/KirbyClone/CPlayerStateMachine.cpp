#include "pch.h"
#include "CPlayerStateMachine.h"
#include "CPlayer.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CAnimation.h"

CPlayerStateMachine::CPlayerStateMachine(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
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
    ChangeState(PLAYER_STATE::IDLE);
}

void CPlayerStateMachine::Update()
{
    if (!m_pOwner || !m_pRigidBody)
        return;

    PLAYER_STATE eNewState = m_eCurState;

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

    case PLAYER_STATE::INHALE_READY:
    case PLAYER_STATE::INHALE_1:
    case PLAYER_STATE::INHALE_2:
    case PLAYER_STATE::INHALE_HOLD:
        UpdateInhaleState();
        break;

    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::FALL:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        UpdateAirborneState();
        break;

    case PLAYER_STATE::EXHALE:
    case PLAYER_STATE::SWALLOW:
        UpdateSpecialState();
        break;
    }
}

void CPlayerStateMachine::ChangeState(PLAYER_STATE _eState)
{
    // 유효한 상태 전환인지 체크
    if (!CanChangeToState(_eState))
        return;

    // 상태 변경
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    // 상태에 맞는 애니메이션 설정
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
            PLAYER_STATE newState = m_pOwner->HasMouthful() ?
                PLAYER_STATE::MOUTHFUL_JUMP : PLAYER_STATE::JUMP;
            ChangeState(newState);
        }
        else
        {
            ChangeState(PLAYER_STATE::FALL);
        }
        return;
    }

    // 땅에 있는 경우 이동 상태 결정
    bool isActuallyMoving = m_pOwner->IsActuallyMoving();
    bool isDecelerating = m_pOwner->IsDecelerating();
    bool hasMouthful = m_pOwner->HasMouthful();
    bool isRunMode = m_pOwner->IsRunMode();

    if (isActuallyMoving || isDecelerating)
    {
        // 실제로 움직이고 있거나 감속 중이면 이동 상태
        if (hasMouthful)
        {
            PLAYER_STATE newState = isRunMode ?
                PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::MOUTHFUL_WALK;
            ChangeState(newState);
        }
        else
        {
            PLAYER_STATE newState = isRunMode ?
                PLAYER_STATE::RUN : PLAYER_STATE::WALK;
            ChangeState(newState);
        }
    }
    else
    {
        // 정지 상태
        PLAYER_STATE newState = hasMouthful ?
            PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
        ChangeState(newState);
    }
}

void CPlayerStateMachine::UpdateInhaleState()
{
    if (!m_pOwner)
        return;

    float inhaleTime = m_pOwner->GetInhaleTime();

    if (!m_pOwner->IsInhaling())
    {
        // 흡입이 끝났으면 상태 변경
        if (!m_pOwner->GetInhaleTargets().empty())
        {
            ChangeState(PLAYER_STATE::SWALLOW);
        }
        else
        {
            ChangeState(PLAYER_STATE::EXHALE);
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
        ChangeState(newState);
    }
}

void CPlayerStateMachine::UpdateAirborneState()
{
    if (!m_pRigidBody)
        return;

    // 땅에 착지했으면 땅 상태로 전환
    if (m_pRigidBody->IsGround())
    {
        bool hasMouthful = m_pOwner->HasMouthful();
        PLAYER_STATE newState = hasMouthful ?
            PLAYER_STATE::MOUTHFUL_IDLE : PLAYER_STATE::IDLE;
        ChangeState(newState);
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
            ChangeState(PLAYER_STATE::IDLE);
            break;

        case PLAYER_STATE::SWALLOW:
            // 삼키기 완료 후 입에 물고 있는 상태로
            if (m_pOwner)
            {
                m_pOwner->SetMouthful(true);
                ChangeState(PLAYER_STATE::MOUTHFUL_IDLE);
            }
            break;
        }
    }
}

void CPlayerStateMachine::SetAnimationForState(PLAYER_STATE _eState)
{
    if (!m_pAnimator)
        return;

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
    case PLAYER_STATE::JUMP:
        m_pAnimator->Play(L"JUMP", false);
        break;
    case PLAYER_STATE::FALL:
        m_pAnimator->Play(L"FALL", true);
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