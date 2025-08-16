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
#include "CPlayerInputManager.h"
#include "CPlayerStateTransitionTable.h"

CPlayerStateMachine::CPlayerStateMachine(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_pInputManager(nullptr)
    , m_pTransitionTable(nullptr)
    , m_eCurState(PLAYER_STATE::IDLE)
    , m_ePrevState(PLAYER_STATE::END)
    , m_fFallTime(0.0f)
    , m_fFallToBounceThreshold(1.0f)
    , m_fBounceHeight(0.6f)
    , m_bWasGrounded(true)
    , m_fSlideTimer(0.0f)
    , m_fSlideDuration(0.8f)
    , m_fSlideDistance(256.f)
    , m_fSlideSpeed(320.f)
    , m_vSlideStartPos(Vec2(0.f, 0.f))
    , m_iSlideDirection(1)
    , m_bSlideGroundCheck(true)
{
    // 새로운 시스템들 생성
    m_pInputManager = new CPlayerInputManager();
    m_pTransitionTable = new CPlayerStateTransitionTable();
}

CPlayerStateMachine::~CPlayerStateMachine()
{
    if (m_pInputManager)
    {
        delete m_pInputManager;
        m_pInputManager = nullptr;
    }

    if (m_pTransitionTable)
    {
        delete m_pTransitionTable;
        m_pTransitionTable = nullptr;
    }
}

void CPlayerStateMachine::Init()
{
    // Movement 시스템에 InputManager 연결
    if (m_pOwner && m_pOwner->GetMovement() && m_pInputManager)
    {
        m_pOwner->GetMovement()->SetInputManager(m_pInputManager);
    }

    // 전환 테이블 초기화
    InitializeTransitionTable();

    // 초기 상태 설정
    m_pOwner->ChangeState(PLAYER_STATE::IDLE);
}

// === 새로운 핵심 업데이트 로직 ===
void CPlayerStateMachine::Update()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!m_pOwner || !pRigidBody || !m_pInputManager || !m_pTransitionTable)
        return;

    // 1. 입력 수집
    m_pInputManager->Update();
    CPlayerInputManager::InputFlags currentInput = m_pInputManager->GetCurrentFrameInput();

    // 2. 상태 전환 체크
    PLAYER_STATE nextState = m_pTransitionTable->GetNextState(m_eCurState, currentInput, m_pOwner);

    // 3. 상태 변경
    if (nextState != m_eCurState)
    {
        ChangeStateInternal(nextState);
    }

    // 4. 현재 상태 실행 (입력 처리 없음, 순수 실행만)
    ExecuteCurrentState();

    // 5. 이전 프레임 Ground 상태 업데이트
    m_bWasGrounded = pRigidBody->IsGround();
}

void CPlayerStateMachine::ChangeStateInternal(PLAYER_STATE _eState)
{
    // 유효성 검사
    if (!CanChangeToState(_eState))
        return;

    // 실제 상태 변경
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    OnStateEnter(_eState);

    // 애니메이션 설정
    SetAnimationForState(_eState);
}

// === 상태 실행 (입력 처리 없음) ===
void CPlayerStateMachine::ExecuteCurrentState()
{
    switch (m_eCurState)
    {
    case PLAYER_STATE::IDLE:
        ExecuteIdleState();
        break;
    case PLAYER_STATE::WALK:
    case PLAYER_STATE::RUN:
    case PLAYER_STATE::MOUTHFUL_IDLE:
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
        ExecuteMovementState();
        break;
    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        ExecuteJumpState();
        break;
    case PLAYER_STATE::FALL:
    case PLAYER_STATE::FALL2:
        ExecuteFallState();
        break;
    case PLAYER_STATE::CROUCH:
        ExecuteCrouchState();
        break;
    case PLAYER_STATE::SLIDE:
        ExecuteSlideState();
        break;
    case PLAYER_STATE::INHALE_READY:
    case PLAYER_STATE::INHALE_1:
    case PLAYER_STATE::INHALE_2:
    case PLAYER_STATE::INHALE_HOLD:
        ExecuteInhaleStates();
        break;
    case PLAYER_STATE::BOUNCE:
        ExecuteBounceState();
        break;
    case PLAYER_STATE::EXHALE:
    case PLAYER_STATE::SWALLOW:
        ExecuteSpecialStates();
        break;
    }
}

void CPlayerStateMachine::ExecuteIdleState()
{
    // IDLE 상태에서는 특별한 처리 없음
    // 이동은 Movement 시스템에서, 상태 전환은 TransitionTable에서 처리
}

void CPlayerStateMachine::ExecuteMovementState()
{
    // 이동 처리는 CPlayerMovement에서 담당
    // 여기서는 상태 관련 물리만 처리

    // 공중에 있으면 전환 테이블에서 처리됨
}

void CPlayerStateMachine::ExecuteJumpState()
{
    // 점프 높이 조절
    if (m_pInputManager && m_pInputManager->IsJumpHold())
    {
        float jumpRatio = m_pInputManager->GetJumpHoldRatio();
        // 점프 높이 조절 로직 (필요시 구현)
    }
}

void CPlayerStateMachine::ExecuteFallState()
{
    // 낙하 시간 누적
    m_fFallTime += CTimeMgr::GetInst()->GetfDT();

    // FALL -> FALL2 전환 (전환 테이블에서 처리하거나 여기서 직접 처리)
    if (m_eCurState == PLAYER_STATE::FALL && m_fFallTime >= m_fFallToBounceThreshold)
    {
        m_pOwner->ChangeState(PLAYER_STATE::FALL2);
    }
}

void CPlayerStateMachine::ExecuteCrouchState()
{
    // 크라우치 상태에서 감속 처리
    if (m_pOwner->GetMovement())
    {
        // Movement 시스템에서 크라우치 감속 처리
    }
}

void CPlayerStateMachine::ExecuteSlideState()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    // 슬라이드 타이머 업데이트
    m_fSlideTimer += CTimeMgr::GetInst()->GetfDT();

    // 지면에서 벗어났는지 체크
    if (m_bSlideGroundCheck && !pRigidBody->IsGround())
    {
        HandleSlideToFall();
        return;
    }

    // 슬라이드 이동 처리
    UpdateSlideMovement();

    // 슬라이드 완료 체크
    CheckSlideCompletion();
}

void CPlayerStateMachine::ExecuteInhaleStates()
{
    // 흡입 상태는 InhaleSystem에서 처리
    // 상태 전환은 TransitionTable에서 처리
}

void CPlayerStateMachine::ExecuteSpecialStates()
{
    // 특수 상태들 (애니메이션 기반)
    CAnimator* pAnimator = m_pOwner ? m_pOwner->GetAnimator() : nullptr;
    if (!pAnimator)
        return;

    CAnimation* pCurAnim = pAnimator->GetCurAnim();
    if (!pCurAnim)
        return;

    // 애니메이션이 끝났는지 체크 후 상태 전환
    if (pCurAnim->IsFinish())
    {
        switch (m_eCurState)
        {
        case PLAYER_STATE::EXHALE:
            m_pOwner->ChangeState(PLAYER_STATE::IDLE);
            break;
        case PLAYER_STATE::SWALLOW:
            m_pOwner->ChangeState(PLAYER_STATE::MOUTHFUL_IDLE);
            break;
        }
    }
}

void CPlayerStateMachine::ExecuteBounceState()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    
    Vec2 vVelocity = pRigidBody->GetVelocity();
    bool isGrounded = pRigidBody->IsGround();

    if (!isGrounded && vVelocity.y >= 0.f)
    {
        m_pOwner->ChangeState(PLAYER_STATE::FALL);
        m_fFallTime = 0.0f;
    }
}

void CPlayerStateMachine::OnStateEnter(PLAYER_STATE _eState)
{
    switch (_eState)
    {
    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        OnEnterJumpState();
        break;
    case PLAYER_STATE::SLIDE:
        OnEnterSlideState();
        break;
    case PLAYER_STATE::INHALE_READY:
        OnEnterInhaleState();
        break;
    case PLAYER_STATE::BOUNCE:
        OnEnterBounceState();
        break;
    case PLAYER_STATE::FALL:
        OnEnterFallState();
        break;
        // 필요한 상태들만 추가
    }
}

void CPlayerStateMachine::OnEnterJumpState()
{
    if (m_pOwner && m_pOwner->GetMovement())
    {
        m_pOwner->GetMovement()->Jump();
    }
}

void CPlayerStateMachine::OnEnterSlideState()
{
    InitiateSlide();
}

void CPlayerStateMachine::OnEnterInhaleState()
{
    if (m_pOwner && m_pOwner->GetInhaleSystem())
    {
        m_pOwner->GetInhaleSystem()->StartInhale();
    }
}

void CPlayerStateMachine::OnEnterBounceState()
{
    PerformBounce();
}

void CPlayerStateMachine::OnEnterFallState()
{
    m_fFallTime = 0.f;
}

// === 전환 테이블 초기화 ===
void CPlayerStateMachine::InitializeTransitionTable()
{
    if (!m_pTransitionTable)
        return;

    // 기본 전환들 추가
    AddBasicMovementTransitions();
    AddJumpAndFallTransitions();
    AddCrouchAndSlideTransitions();
    AddInhaleTransitions();
    AddSpecialTransitions();

    char debugMsg[128];
    sprintf_s(debugMsg, "StateMachine: Initialized %d transitions\n",
        m_pTransitionTable->GetTransitionCount());
    OutputDebugStringA(debugMsg);
}

void CPlayerStateMachine::AddBasicMovementTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // IDLE <-> WALK 전환
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::IDLE,
        (uint32_t)INPUT::MOVE_LEFT | (uint32_t)INPUT::MOVE_RIGHT,
        PLAYER_STATE::WALK,
        [](CPlayer* p) {
            return p->GetRigidBody() && p->GetRigidBody()->IsGround();
        },
        100
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::WALK,
        0,  // 어떤 입력도 필요하지 않음
        PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;

            // 입력이 없고, 실제로 움직이지 않고, 감속도 완료되었을 때만 IDLE로 전환
            CPlayerInputManager* pInputMgr = p->GetStateMachine()->GetInputManager();
            if (!pInputMgr) return false;

            bool hasLeftInput = pInputMgr->IsMovingLeft();
            bool hasRightInput = pInputMgr->IsMovingRight();
            bool isMoving = p->GetMovement()->IsActuallyMoving();
            bool isDecelerating = p->GetMovement()->IsDecelerating();

            // 핵심: 입력도 없고, 움직이지도 않고, 감속도 끝났을 때만 IDLE
            return !hasLeftInput && !hasRightInput && !isMoving && !isDecelerating;
        },
        50,
        // 금지된 입력: 방향키가 눌려있으면 전환하지 않음
        (uint32_t)INPUT::MOVE_LEFT | (uint32_t)INPUT::MOVE_RIGHT
    );

    // 더블탭 RUN 전환
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::WALK,
        (uint32_t)INPUT::DOUBLE_TAP_LEFT | (uint32_t)INPUT::DOUBLE_TAP_RIGHT,
        PLAYER_STATE::RUN,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        150
    );

    // RUN -> WALK (더블탭 모드 해제)
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::RUN,
        (uint32_t)INPUT::MOVE_LEFT | (uint32_t)INPUT::MOVE_RIGHT,
        PLAYER_STATE::WALK,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;
            return !p->GetMovement()->IsRunMode();
        },
        80
    );

    // RUN -> IDLE (이동 입력 없음)
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::RUN,
        0,
        PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            if (!p->GetMovement()) return false;
            // 실제로 움직이지 않을 때
            return !p->GetMovement()->IsActuallyMoving() &&
                !p->GetMovement()->IsDecelerating();
        },
        60
    );
}

void CPlayerStateMachine::AddJumpAndFallTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 점프 시작
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::IDLE,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::JUMP,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        200
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::WALK,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::JUMP,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        200
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::RUN,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::JUMP,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        200
    );

    // 점프 -> 낙하
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::JUMP,
        0,
        PLAYER_STATE::FALL,
        [this](CPlayer* p) {
            if (!p->GetRigidBody()) return false;
            // 점프 후 충분한 시간이 지났고 하강 중일 때만
            return p->GetRigidBody()->GetVelocity().y > 50.f;
        },
        300
    );

    // 낙하 -> 착지
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::FALL,
        0,
        PLAYER_STATE::IDLE,
        [this](CPlayer* p) {
            if (!p->GetRigidBody()) return false;
            // 땅에 닿았고 이전 프레임에는 공중에 있었을 때만
            return p->GetRigidBody()->IsGround() && !m_bWasGrounded;
        },
        300
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::FALL2,
        0,
        PLAYER_STATE::BOUNCE,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        350
    );

    // 지상 -> 낙하
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::CROUCH,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) { return p->GetRigidBody() && !p->GetRigidBody()->IsGround(); },
        400
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::IDLE,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) { return p->GetRigidBody() && !p->GetRigidBody()->IsGround(); },
        400
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::WALK,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) { return p->GetRigidBody() && !p->GetRigidBody()->IsGround(); },
        400
    );

    m_pTransitionTable->AddTransition(
        PLAYER_STATE::RUN,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) { return p->GetRigidBody() && !p->GetRigidBody()->IsGround(); },
        400
    );
}

void CPlayerStateMachine::AddCrouchAndSlideTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 크라우치 진입
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::IDLE,
        (uint32_t)INPUT::MOVE_DOWN,
        PLAYER_STATE::CROUCH,
        [](CPlayer* p) {
            return p->GetRigidBody() && p->GetRigidBody()->IsGround() &&
                !p->HasMouthful();
        },
        150
    );

    // 크라우치 해제
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::CROUCH,
        0,  // DOWN 키가 없을 때
        PLAYER_STATE::IDLE,
        [this](CPlayer* p) {
            // InputManager에서 직접 체크
            if (!m_pInputManager) return false;
            return !m_pInputManager->IsMovingDown();
        },
        100
    );

    // 슬라이드 시작
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::CROUCH,
        (uint32_t)INPUT::JUMP_TAP | (uint32_t)INPUT::ACTION_TAP,
        PLAYER_STATE::SLIDE,
        [](CPlayer* p) { return p->GetRigidBody() && p->GetRigidBody()->IsGround(); },
        250
    );

    // 슬라이드 -> 낙하 (공중으로 떨어짐)
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::SLIDE,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) { return p->GetRigidBody() && !p->GetRigidBody()->IsGround(); },
        400
    );
}

void CPlayerStateMachine::AddInhaleTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 흡입 시작
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::IDLE,
        (uint32_t)INPUT::ACTION_TAP,
        PLAYER_STATE::INHALE_READY,
        [](CPlayer* p) { return !p->HasMouthful(); },
        180
    );

    // 흡입 종료 -> 내뱉기
    m_pTransitionTable->AddTransition(
        PLAYER_STATE::INHALE_HOLD,
        (uint32_t)INPUT::ACTION_AWAY,
        PLAYER_STATE::EXHALE,
        nullptr,
        300
    );
}

void CPlayerStateMachine::AddSpecialTransitions()
{
    // 특수 전환들 (머금은 상태 등)
    // 필요시 추가
}

// === 기존 슬라이드 관련 함수들 (변경 없음) ===
void CPlayerStateMachine::InitiateSlide()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!m_pOwner || !pRigidBody)
        return;

    m_fSlideTimer = 0.0f;
    m_vSlideStartPos = m_pOwner->GetPos();
    m_bSlideGroundCheck = true;

    CPlayerMovement* pMovement = m_pOwner->GetMovement();
    if (pMovement)
    {
        m_iSlideDirection = pMovement->IsFacingRight() ? 1 : -1;
    }
    else
    {
        m_iSlideDirection = 1;
    }

    pRigidBody->SetVelocityX(m_fSlideSpeed * m_iSlideDirection);
}

void CPlayerStateMachine::UpdateSlideMovement()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    float slideProgress = m_fSlideTimer / m_fSlideDuration;

    if (slideProgress <= 1.0f)
    {
        float speedMultiplier = 1.0f - (slideProgress * slideProgress);
        float currentSlideSpeed = m_fSlideSpeed * speedMultiplier;
        currentSlideSpeed = max(currentSlideSpeed, m_fSlideSpeed * 0.3f);

        pRigidBody->SetVelocityX(currentSlideSpeed * m_iSlideDirection);
    }
}

void CPlayerStateMachine::CheckSlideCompletion()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!m_pOwner || !pRigidBody)
        return;

    bool shouldEndSlide = false;

    if (m_fSlideTimer >= m_fSlideDuration)
        shouldEndSlide = true;

    Vec2 currentPos = m_pOwner->GetPos();
    float distanceTraveled = abs(currentPos.x - m_vSlideStartPos.x);
    if (distanceTraveled >= m_fSlideDistance)
        shouldEndSlide = true;

    float currentSpeedX = abs(pRigidBody->GetVelocity().x);
    if (currentSpeedX < 50.f)
        shouldEndSlide = true;

    if (shouldEndSlide)
    {
        pRigidBody->SetVelocityX(0.f);

        if (m_pInputManager && m_pInputManager->IsMovingDown())
        {
            m_pOwner->ChangeState(PLAYER_STATE::CROUCH);
        }
        else
        {
            m_pOwner->ChangeState(PLAYER_STATE::IDLE);
        }
    }
}

void CPlayerStateMachine::HandleSlideToFall()
{
    if (!m_pOwner)
        return;

    m_pOwner->ChangeState(PLAYER_STATE::FALL);
    m_fFallTime = 0.0f;
    m_fSlideTimer = 0.0f;
    m_bSlideGroundCheck = false;
}

// === 기존 함수들 (변경 없음) ===
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

void CPlayerStateMachine::HandleLanding()
{
    // 전환 테이블에서 처리됨
}

void CPlayerStateMachine::PerformBounce()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!m_pOwner || !pRigidBody)
        return;

    CPlayerMovement* pMovement = m_pOwner->GetMovement();
    if (pMovement)
    {
        float baseJumpPower = pMovement->GetJumpPower();
        float bounceJumpPower = baseJumpPower * m_fBounceHeight;

        pRigidBody->SetVelocityY(-bounceJumpPower);
        pRigidBody->SetGround(false);
    }
}

void CPlayerStateMachine::SetAnimationForState(PLAYER_STATE _eState)
{
    CAnimator* pAnimator = m_pOwner ? m_pOwner->GetAnimator() : nullptr;
    if (!pAnimator)
        return;

    switch (_eState)
    {
    case PLAYER_STATE::IDLE:
        pAnimator->Play(L"IDLE", true);
        break;
    case PLAYER_STATE::WALK:
        pAnimator->Play(L"WALK", true);
        break;
    case PLAYER_STATE::RUN:
        pAnimator->Play(L"RUN", true);
        break;
    case PLAYER_STATE::CROUCH:
        pAnimator->Play(L"CROUCH", true);
        break;
    case PLAYER_STATE::SLIDE:
        pAnimator->Play(L"SLIDE", false);
        break;
    case PLAYER_STATE::JUMP:
        pAnimator->Play(L"JUMP", false);
        break;
    case PLAYER_STATE::FALL:
        pAnimator->Play(L"FALL", true);
        break;
    case PLAYER_STATE::FALL2:
        pAnimator->Play(L"FALL2", true);
        break;
    case PLAYER_STATE::BOUNCE:
        pAnimator->Play(L"BOUNCE", false);
        break;
    case PLAYER_STATE::INHALE_READY:
        pAnimator->Play(L"INHALE_READY", false);
        break;
    case PLAYER_STATE::INHALE_1:
        pAnimator->Play(L"INHALE_1", true);
        break;
    case PLAYER_STATE::INHALE_2:
        pAnimator->Play(L"INHALE_2", true);
        break;
    case PLAYER_STATE::INHALE_HOLD:
        pAnimator->Play(L"INHALE_HOLD", true);
        break;
    case PLAYER_STATE::EXHALE:
        pAnimator->Play(L"EXHALE", false);
        break;
    case PLAYER_STATE::SWALLOW:
        pAnimator->Play(L"SWALLOW", false);
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        pAnimator->Play(L"MOUTHFUL_IDLE", true);
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
        pAnimator->Play(L"MOUTHFUL_WALK", true);
        break;
    case PLAYER_STATE::MOUTHFUL_RUN:
        pAnimator->Play(L"MOUTHFUL_RUN", true);
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        pAnimator->Play(L"MOUTHFUL_JUMP", false);
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

    case PLAYER_STATE::SLIDE:
        // 슬라이드 중에는 FALL 상태로만 전환 가능 (공중에 떨어질 때)
        return (_to == PLAYER_STATE::FALL ||
            _to == PLAYER_STATE::CROUCH ||
            _to == PLAYER_STATE::IDLE);
    }

    // 기본적으로 모든 전환 허용
    return true;
}