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
#include "CPlayerHealthSystem.h"

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
    , m_eHoverSubState(HOVER_SUBSTATE::END)
    , m_fHoverUpForce(300.f)            // 상승력 조금 줄임
    , m_fHoverFallSpeed(80.f)           // 천천히 낙하하는 속도
    , m_fHoverMoveSpeed(120.f)          // 걷기 속도 정도
    , m_fHoverSubStateTimer(0.0f)
    , m_fHoverEnterDuration(0.6f)       // ENTER 지속 시간 늘림
    , m_fHoverFlyUpDuration(0.4f)       // FLY_UP 지속 시간 늘림
    , m_fHoverAirFriction(5.0f)         // 공중 마찰 계수
    , m_bHoverCanMoveOnGround(true)     // 땅에서도 이동 가능
    , m_fDamageTimer(0.0f)          // NEW!
    , m_fDamageDuration(0.5f)       // NEW! 0.5초 피격 상태
    , m_bDamageCompleted(false)
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
    ChangeStateInternal(PLAYER_STATE::IDLE);
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
    case PLAYER_STATE::HOVER:
        ExecuteHoverState();
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        m_fHoverSubStateTimer += CTimeMgr::GetInst()->GetfDT();
        ExecuteHoverExhaleState();
        break;
    case PLAYER_STATE::SLIDE:
        ExecuteSlideState();
        break;
    case PLAYER_STATE::DAMAGE:
        ExecuteDamageState();
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

    // 슬라이드 이동 처리
    UpdateSlideMovement();

    // 슬라이드 완료 체크 (속도 정리만)
    CheckSlideCompletion();

    // 전환은 TransitionTable에서 자동 처리됨
}

void CPlayerStateMachine::ExecuteInhaleStates()
{
    // 흡입 상태는 InhaleSystem에서 처리
    // 상태 전환은 TransitionTable에서 처리
}

void CPlayerStateMachine::ExecuteSpecialStates()
{
}

void CPlayerStateMachine::ExecuteBounceState()
{
}

void CPlayerStateMachine::ExecuteHoverState()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody || !m_pInputManager)
        return;

    // 서브스테이트 타이머 업데이트
    m_fHoverSubStateTimer += CTimeMgr::GetInst()->GetfDT();

    // 현재 서브스테이트에 따른 처리
    switch (m_eHoverSubState)
    {
    case HOVER_SUBSTATE::ENTER:
        UpdateHoverEnter();
        break;
    case HOVER_SUBSTATE::FLY_UP:
        UpdateHoverFlyUp();
        break;
    case HOVER_SUBSTATE::FLOAT:
        UpdateHoverFloat();
        break;
    case HOVER_SUBSTATE::GROUNDED:
        UpdateHoverGrounded();
        break;
    }

    // 공통 처리
    UpdateHoverMovement();
    UpdateHoverPhysics();
}

void CPlayerStateMachine::ExecuteHoverExhaleState()
{
}

void CPlayerStateMachine::ExecuteDamageState()
{
    // 피격 타이머 업데이트
    m_fDamageTimer += CTimeMgr::GetInst()->GetfDT();

    // 피격 시간이 끝나면 완료 플래그 설정 (전환은 테이블에서)
    if (m_fDamageTimer >= m_fDamageDuration)
    {
        m_bDamageCompleted = true;
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
    case PLAYER_STATE::HOVER:
        OnEnterHoverState();
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        OnEnterHoverExhaleState();
        break;
    case PLAYER_STATE::DAMAGE:
        OnEnterDamageState();
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

void CPlayerStateMachine::OnEnterHoverState()
{
    // HOVER 상태 진입 시 초기화
    m_eHoverSubState = HOVER_SUBSTATE::ENTER;
    ResetHoverSubStateTimer();

    // 중력 비활성화
    DisableGravityForHover();
}

void CPlayerStateMachine::OnEnterHoverExhaleState()
{
    // 타이머 초기화 (백업 전환용)
    m_fHoverSubStateTimer = 0.0f;

    // HOVER 종료 시 중력 재활성화
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetUseGravity(true);
    }
}

void CPlayerStateMachine::OnExitHoverState()
{
    // HOVER 완전 종료 시 정리
    RestoreGravityFromHover();
    m_eHoverSubState = HOVER_SUBSTATE::END;
}

void CPlayerStateMachine::OnEnterDamageState()
{
    m_fDamageTimer = 0.0f;          // 타이머 초기화
    m_bDamageCompleted = false;     // 완료 플래그 초기화

    // 피격 플래그 정리
    if (m_pOwner)
    {
        m_pOwner->ClearDamageRequest();
    }
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
    AddHoverTransitions();
    AddDamageTransitions();
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

    // === FALL -> FALL2 전환 (시간 기반) ===
    m_pTransitionTable->AddTransition(PLAYER_STATE::FALL, 0, PLAYER_STATE::FALL2,
        [this](CPlayer* p) {
            // 낙하 시간이 임계값에 도달했을 때
            return m_fFallTime >= m_fFallToBounceThreshold;
        },
        320);  // 일반 FALL 전환보다 높은 우선순위

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

    // Ground 상태 불일치 감지 전환
    std::vector<PLAYER_STATE> groundStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN,
        PLAYER_STATE::CROUCH
    };

    for (PLAYER_STATE state : groundStates)
    {
        m_pTransitionTable->AddTransition(state, 0, PLAYER_STATE::FALL,
            [](CPlayer* p) {
                CRigidBody* pRB = p->GetRigidBody();
                return pRB && !pRB->IsGround();
            },
            500);  // 높은 우선순위
    }
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

    // === 슬라이드 완료 전환들 ===

    // SLIDE -> CROUCH (DOWN 키 유지 중이고 슬라이드 완료)
    m_pTransitionTable->AddTransition(PLAYER_STATE::SLIDE, 0, PLAYER_STATE::CROUCH,
        [this](CPlayer* p) {
            if (!IsSlideCompleted()) return false;
            return m_pInputManager && m_pInputManager->IsMovingDown();
        },
        280);

    // SLIDE -> IDLE (슬라이드 완료, DOWN 키 없음)
    m_pTransitionTable->AddTransition(PLAYER_STATE::SLIDE, 0, PLAYER_STATE::IDLE,
        [this](CPlayer* p) {
            if (!IsSlideCompleted()) return false;
            return !m_pInputManager || !m_pInputManager->IsMovingDown();
        },
        270);

    // SLIDE -> FALL (지면에서 벗어남)
    m_pTransitionTable->AddTransition(PLAYER_STATE::SLIDE, 0, PLAYER_STATE::FALL,
        [this](CPlayer* p) {
            CRigidBody* pRB = p->GetRigidBody();
            return pRB && !pRB->IsGround() && m_bSlideGroundCheck;
        },
        400);  // 높은 우선순위
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
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // === 애니메이션 기반 전환들 ===

    // EXHALE -> IDLE (애니메이션 완료 시)
    m_pTransitionTable->AddTransition(PLAYER_STATE::EXHALE, 0, PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            CAnimator* pAnimator = p->GetAnimator();
            if (!pAnimator) return false;
            CAnimation* pCurAnim = pAnimator->GetCurAnim();
            return pCurAnim && pCurAnim->IsFinish();
        },
        400);  // 높은 우선순위

    // SWALLOW -> MOUTHFUL_IDLE (애니메이션 완료 시)
    m_pTransitionTable->AddTransition(PLAYER_STATE::SWALLOW, 0, PLAYER_STATE::MOUTHFUL_IDLE,
        [](CPlayer* p) {
            CAnimator* pAnimator = p->GetAnimator();
            if (!pAnimator) return false;
            CAnimation* pCurAnim = pAnimator->GetCurAnim();
            return pCurAnim && pCurAnim->IsFinish();
        },
        400);

    // BOUNCE -> FALL (공중에서 상승 끝날 때)
    m_pTransitionTable->AddTransition(PLAYER_STATE::BOUNCE, 0, PLAYER_STATE::FALL,
        [this](CPlayer* p) {
            CRigidBody* pRB = p->GetRigidBody();
            if (!pRB) return false;
            Vec2 vVelocity = pRB->GetVelocity();
            bool isGrounded = pRB->IsGround();

            // 공중에 있고 하강 시작했을 때
            return !isGrounded && vVelocity.y >= 0.f;
        },
        350);
}

void CPlayerStateMachine::AddHoverTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // === HOVER 진입 전환 ===

    // JUMP -> HOVER (공중에서 Z키)
    m_pTransitionTable->AddTransition(PLAYER_STATE::JUMP,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::HOVER,
        [](CPlayer* p) {
            return p->GetRigidBody() && !p->GetRigidBody()->IsGround();
        },
        250);

    // FALL -> HOVER (낙하 중 Z키)
    m_pTransitionTable->AddTransition(PLAYER_STATE::FALL,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::HOVER,
        nullptr,
        250);

    // FALL2 -> HOVER (장시간 낙하 중 Z키)
    m_pTransitionTable->AddTransition(PLAYER_STATE::FALL2,
        (uint32_t)INPUT::JUMP_TAP,
        PLAYER_STATE::HOVER,
        nullptr,
        250);

    // HOVER -> HOVER_EXHALE (X키 입력으로 내뱉기)
    m_pTransitionTable->AddTransition(PLAYER_STATE::HOVER,
        (uint32_t)INPUT::ACTION_TAP,
        PLAYER_STATE::HOVER_EXHALE,
        nullptr,
        400);  // 높은 우선순위

    // === HOVER 종료 전환 ===

    // HOVER_EXHALE -> IDLE (마지막 프레임 즉시 전환)
    m_pTransitionTable->AddTransition(PLAYER_STATE::HOVER_EXHALE,
        0,
        PLAYER_STATE::IDLE,
        [](CPlayer* p) {
            CAnimator* pAnim = p->GetAnimator();
            CRigidBody* pRigidBody = p->GetRigidBody();

            if (!pAnim || !pRigidBody) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim();
            if (!pCurAnim) return false;

            // === 핵심 수정: 마지막 프레임 도달 시 즉시 전환 ===
            bool animFinished = pCurAnim->IsFinish();
            bool lastFrameReached = (pCurAnim->GetCurFrame() >= pCurAnim->GetMaxFrame() - 1);
            bool isGrounded = pRigidBody->IsGround();

            // 애니메이션이 끝났거나 마지막 프레임에 도달했으면 전환
            bool shouldTransition = (animFinished || lastFrameReached) && isGrounded;

            return shouldTransition;
        },
        350);

    // HOVER_EXHALE -> FALL (마지막 프레임 즉시 전환)
    m_pTransitionTable->AddTransition(PLAYER_STATE::HOVER_EXHALE,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) {
            CAnimator* pAnim = p->GetAnimator();
            CRigidBody* pRigidBody = p->GetRigidBody();

            if (!pAnim || !pRigidBody) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim();
            if (!pCurAnim) return false;

            // === 핵심 수정: 마지막 프레임 도달 시 즉시 전환 ===
            bool animFinished = pCurAnim->IsFinish();
            bool lastFrameReached = (pCurAnim->GetCurFrame() >= pCurAnim->GetMaxFrame() - 1);
            bool isAirborne = !pRigidBody->IsGround();

            // 애니메이션이 끝났거나 마지막 프레임에 도달했으면 전환
            bool shouldTransition = (animFinished || lastFrameReached) && isAirborne;

            return shouldTransition;
        },
        340);

    // === 백업 전환 (시간 기반) ===
    // 애니메이션이 제대로 끝나지 않을 경우를 대비한 시간 기반 전환

    // HOVER_EXHALE -> FALL (시간 기반 백업, 낮은 우선순위)
    m_pTransitionTable->AddTransition(PLAYER_STATE::HOVER_EXHALE,
        0, // 특정 입력 없음
        PLAYER_STATE::FALL,
        [this](CPlayer* p) {
            // 1초 이상 HOVER_EXHALE 상태라면 강제 전환
            if (m_fHoverSubStateTimer > 1.0f) {
                return true;
            }
            return false;
        },
        100);  // 매우 낮은 우선순위 (백업용)
}

void CPlayerStateMachine::AddDamageTransitions()
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // === 피격 조건 정의 ===
    auto damageCondition = [](CPlayer* p) -> bool {
        if (!p) return false;

        // 게임오버 상태면 피격 불가
        
        if (p->GetHealthSystem() && p->GetHealthSystem()->IsGameOver())
            return false;

        // 피격 요청 플래그 체크
        return p->IsDamageRequested();
    };

    // === 모든 상태에서 DAMAGE로의 전환 (기존과 동일) ===
    m_pTransitionTable->AddTransition(PLAYER_STATE::IDLE, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::WALK, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::RUN, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::JUMP, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::FALL, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::FALL2, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::CROUCH, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::SLIDE, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::HOVER, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::INHALE_READY, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::INHALE_1, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::INHALE_2, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::INHALE_HOLD, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::MOUTHFUL_IDLE, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::MOUTHFUL_WALK, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::MOUTHFUL_RUN, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);
    m_pTransitionTable->AddTransition(PLAYER_STATE::MOUTHFUL_JUMP, 0, PLAYER_STATE::DAMAGE, damageCondition, 2000);

    // === DAMAGE 상태에서의 전환 (완료 플래그 기반) ===

    // DAMAGE -> IDLE (땅에 있고 시간 완료)
    m_pTransitionTable->AddTransition(PLAYER_STATE::DAMAGE,
        0,
        PLAYER_STATE::IDLE,
        [](CPlayer* p) -> bool {
            if (!p || !p->GetRigidBody() || !p->GetStateMachine())
                return false;

            // 피격 시간이 완료되고 땅에 있을 때
            return p->GetStateMachine()->IsDamageCompleted() &&
                p->GetRigidBody()->IsGround();
        },
        1000
    );

    // DAMAGE -> FALL (공중에 있고 시간 완료)  
    m_pTransitionTable->AddTransition(PLAYER_STATE::DAMAGE,
        0,
        PLAYER_STATE::FALL,
        [](CPlayer* p) -> bool {
            if (!p || !p->GetRigidBody() || !p->GetStateMachine())
                return false;

            // 피격 시간이 완료되고 공중에 있을 때
            return p->GetStateMachine()->IsDamageCompleted() &&
                !p->GetRigidBody()->IsGround();
        },
        1000
    );

    // === 입력 무시 규칙들 ===
    std::vector<uint32_t> ignoredInputs = {
        (uint32_t)INPUT::JUMP_TAP,
        (uint32_t)INPUT::ACTION_TAP,
        (uint32_t)INPUT::MOVE_LEFT,
        (uint32_t)INPUT::MOVE_RIGHT,
        (uint32_t)INPUT::MOVE_DOWN,
        (uint32_t)INPUT::DOUBLE_TAP_LEFT,
        (uint32_t)INPUT::DOUBLE_TAP_RIGHT
    };

    for (uint32_t input : ignoredInputs)
    {
        m_pTransitionTable->AddTransition(PLAYER_STATE::DAMAGE,
            input,
            PLAYER_STATE::DAMAGE,  // 자기 자신으로 전환 (입력 무시)
            nullptr,
            900  // IDLE/FALL 전환보다는 낮지만 높은 우선순위
        );
    }
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
    // 이 함수는 더 이상 상태 전환을 하지 않음
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return;

    // 슬라이드 완료 시 속도만 정리 (상태 전환은 TransitionTable에서)
    if (IsSlideCompleted())
    {
        pRigidBody->SetVelocityX(0.f);
        // 상태 전환은 TransitionTable에서 자동으로 처리됨
    }
}

void CPlayerStateMachine::HandleSlideToFall()
{
    // 이 함수도 더 이상 상태 전환을 하지 않음
    // 필요한 플래그만 설정
    m_fFallTime = 0.0f;
    m_fSlideTimer = 0.0f;
    m_bSlideGroundCheck = false;

    // 상태 전환은 TransitionTable에서 자동으로 처리됨
}

bool CPlayerStateMachine::IsSlideCompleted() const
{
    if (m_eCurState != PLAYER_STATE::SLIDE) return false;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return true;

    // 슬라이드 완료 조건들
    bool timeCompleted = (m_fSlideTimer >= m_fSlideDuration);

    Vec2 currentPos = m_pOwner->GetPos();
    float distanceTraveled = abs(currentPos.x - m_vSlideStartPos.x);
    bool distanceCompleted = (distanceTraveled >= m_fSlideDistance);

    float currentSpeedX = abs(pRigidBody->GetVelocity().x);
    bool speedTooSlow = (currentSpeedX < 50.f);

    return timeCompleted || distanceCompleted || speedTooSlow;
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

bool CPlayerStateMachine::IsHoverState() const
{
    return (m_eCurState == PLAYER_STATE::HOVER ||
        m_eCurState == PLAYER_STATE::HOVER_EXHALE);
}

bool CPlayerStateMachine::IsHoverGrounded() const
{
    return (m_eCurState == PLAYER_STATE::HOVER &&
        m_eHoverSubState == HOVER_SUBSTATE::GROUNDED);
}

bool CPlayerStateMachine::IsHoverFloating() const
{
    return (m_eCurState == PLAYER_STATE::HOVER &&
        (m_eHoverSubState == HOVER_SUBSTATE::FLOAT ||
            m_eHoverSubState == HOVER_SUBSTATE::FLY_UP));
}

void CPlayerStateMachine::ForceStateChange(PLAYER_STATE _eState)
{
    // 안전 검사 후 상태 변경
    if (IsValidStateTransition(m_eCurState, _eState))
    {
        ChangeStateInternal(_eState);
        // 즉시 한 번 실행이 필요하다면 여기서만
    }
}

void CPlayerStateMachine::ForceStateForSystemReset(PLAYER_STATE _eState)
{
    // 시스템 리셋 시에만 사용 (게임오버, 스테이지 재시작 등)
    if (_eState == PLAYER_STATE::IDLE ||
        _eState == PLAYER_STATE::END)  // 허용된 시스템 상태들만
    {
        ChangeStateInternal(_eState);

        // 리셋 시 필요한 추가 처리
        m_fFallTime = 0.0f;
        m_fSlideTimer = 0.0f;
        m_fDamageTimer = 0.0f;
        m_bDamageCompleted = false;
        // 기타 타이머들 초기화
    }
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

void CPlayerStateMachine::UpdateHoverEnter()
{
    // 초기 상승력 적용 (정확히 한 번만)
    if (m_fHoverSubStateTimer < 0.05f)
    {
        ApplyHoverUpForce();
    }

    // 일정 시간 후 또는 상승이 멈추면 FLOAT로 전환
    if (m_fHoverSubStateTimer >= m_fHoverEnterDuration)
    {
        ChangeHoverSubState(HOVER_SUBSTATE::FLOAT);
    }
    else
    {
        // 상승 속도가 0에 가까워지면 조기에 FLOAT로 전환
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            Vec2 vVelocity = pRigidBody->GetVelocity();
            if (vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.2f) // 거의 멈췄고 최소 시간 지남
            {
                ChangeHoverSubState(HOVER_SUBSTATE::FLOAT);
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFlyUp()
{
    // 상승력 적용 (정확히 한 번만)
    if (m_fHoverSubStateTimer < 0.05f)
    {
        ApplyHoverUpForce();
    }

    // 일정 시간 후 또는 상승이 멈추면 FLOAT로 전환
    if (m_fHoverSubStateTimer >= m_fHoverFlyUpDuration)
    {
        ChangeHoverSubState(HOVER_SUBSTATE::FLOAT);
    }
    else
    {
        // 상승 속도가 0에 가까워지면 조기에 FLOAT로 전환
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            Vec2 vVelocity = pRigidBody->GetVelocity();
            if (vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.1f)
            {
                ChangeHoverSubState(HOVER_SUBSTATE::FLOAT);
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFloat()
{
    // Z키 입력 시 FLY_UP으로 전환
    if (m_pInputManager->IsJumpTap())
    {
        ChangeHoverSubState(HOVER_SUBSTATE::FLY_UP);
    }

    // 땅에 닿으면 GROUNDED로 전환
    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (pRigidBody && pRigidBody->IsGround())
    {
        ChangeHoverSubState(HOVER_SUBSTATE::GROUNDED);
    }
}

void CPlayerStateMachine::UpdateHoverGrounded()
{
    // Z키 입력이 있으면 FLY_UP으로 전환
    if (m_pInputManager->IsJumpTap())
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            pRigidBody->SetGround(false); // 땅에서 떠오르기
        }
        ChangeHoverSubState(HOVER_SUBSTATE::FLY_UP);
    }
}

void CPlayerStateMachine::UpdateHoverMovement()
{
    if (!m_pInputManager) return;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return;

    // 땅에서 이동 불가 설정이면 GROUNDED 상태에서 이동 제한
    if (!m_bHoverCanMoveOnGround && m_eHoverSubState == HOVER_SUBSTATE::GROUNDED)
        return;

    // 좌우 이동 입력 처리
    int horizontalInput = m_pInputManager->GetHorizontalInput();
    if (horizontalInput != 0)
    {
        float moveVelocity = m_fHoverMoveSpeed * horizontalInput;
        pRigidBody->SetVelocityX(moveVelocity);

        // 방향 업데이트
        if (m_pOwner->GetMovement())
        {
            m_pOwner->GetMovement()->SetFacingDirection(horizontalInput > 0);
        }
    }
    else
    {
        // 입력이 없으면 X축 속도 감속 (공중 마찰)
        Vec2 vVel = pRigidBody->GetVelocity();
        float deltaTime = CTimeMgr::GetInst()->GetfDT();
        float newVelX = vVel.x * (1.0f - m_fHoverAirFriction * deltaTime);

        if (abs(newVelX) < 10.f) newVelX = 0.f;
        pRigidBody->SetVelocityX(newVelX);
    }
}

void CPlayerStateMachine::UpdateHoverPhysics()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return;

    // 서브스테이트에 따른 물리 처리
    switch (m_eHoverSubState)
    {
    case HOVER_SUBSTATE::ENTER:
        // ENTER 상태에서는 초기 상승 후 서서히 감속
    {
        Vec2 vVelocity = pRigidBody->GetVelocity();
        if (vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f) // 상승 중이고 초기 시간 지남
        {
            // 상승 속도를 서서히 감소시킴
            float deceleration = 600.f; // 감속도
            float deltaTime = CTimeMgr::GetInst()->GetfDT();
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if (newVelY > 0) newVelY = 0; // 하강은 하지 않음
            pRigidBody->SetVelocityY(newVelY);
        }
    }
    break;

    case HOVER_SUBSTATE::FLY_UP:
        // FLY_UP 상태에서도 같은 처리
    {
        Vec2 vVelocity = pRigidBody->GetVelocity();
        if (vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f)
        {
            float deceleration = 600.f;
            float deltaTime = CTimeMgr::GetInst()->GetfDT();
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if (newVelY > 0) newVelY = 0;
            pRigidBody->SetVelocityY(newVelY);
        }
    }
    break;

    case HOVER_SUBSTATE::FLOAT:
        // FLOAT 상태에서는 천천히 낙하
        ApplyHoverFallSpeed();
        break;

    case HOVER_SUBSTATE::GROUNDED:
        // GROUNDED 상태에서는 낙하 없음 (땅에 붙어있음)
    {
        Vec2 vVelocity = pRigidBody->GetVelocity();
        if (vVelocity.y > 0) // 하강 중이면 멈춤
        {
            pRigidBody->SetVelocityY(0);
        }
    }
    break;
    }
}

void CPlayerStateMachine::ApplyHoverUpForce()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetVelocityY(-m_fHoverUpForce);
    }
}

void CPlayerStateMachine::ApplyHoverFallSpeed()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody) return;

    Vec2 vVelocity = pRigidBody->GetVelocity();

    // 현재 Y 속도가 HOVER 낙하 속도보다 작으면 (느리면) 낙하 속도로 설정
    if (vVelocity.y < m_fHoverFallSpeed)
    {
        // 서서히 낙하 속도까지 증가
        float deltaTime = CTimeMgr::GetInst()->GetfDT();
        float acceleration = 200.f; // 낙하 가속도
        float newVelY = vVelocity.y + acceleration * deltaTime;

        // 최대 낙하 속도 제한
        if (newVelY > m_fHoverFallSpeed)
        {
            newVelY = m_fHoverFallSpeed;
        }

        pRigidBody->SetVelocityY(newVelY);
    }
}

void CPlayerStateMachine::DisableGravityForHover()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetUseGravity(false);
    }
}

void CPlayerStateMachine::RestoreGravityFromHover()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetUseGravity(true);
    }
}

void CPlayerStateMachine::ChangeHoverSubState(HOVER_SUBSTATE _eNewSubState)
{
    if (m_eHoverSubState == _eNewSubState)
        return;

    m_eHoverSubState = _eNewSubState;
    ResetHoverSubStateTimer();

    // 현재 HOVER 상태라면 애니메이션 재설정
    if (m_eCurState == PLAYER_STATE::HOVER)
    {
        SetAnimationForState(PLAYER_STATE::HOVER);
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
    case PLAYER_STATE::HOVER_EXHALE:
        pAnimator->Play(L"HOVER_EXHALE", false);          // HOVER 종료 (내뱉기)
        break;
    case PLAYER_STATE::HOVER:
        switch (m_eHoverSubState)
        {
        case HOVER_SUBSTATE::ENTER:
            pAnimator->Play(L"HOVER_ENTER", false);     // 공기 머금기 (한 번만)
            break;
        case HOVER_SUBSTATE::FLY_UP:
            pAnimator->Play(L"HOVER_FLY", false);       // 버둥거리기 (한 번만)
            break;
        case HOVER_SUBSTATE::FLOAT:
            pAnimator->Play(L"HOVER_FLOAT", true);      // 떠다니기 (반복)
            break;
        case HOVER_SUBSTATE::GROUNDED:
            pAnimator->Play(L"HOVER_GROUND", true);     // 땅에서 떠다니기 (반복)
            break;
        default:
            pAnimator->Play(L"HOVER_FLOAT", true);      // 기본값
            break;
        }
        break;
    case PLAYER_STATE::DAMAGE:
        pAnimator->Play(L"DAMAGE", false);
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

// IsValidStateTransition - HOVER_EXHALE 제한 완전 제거
bool CPlayerStateMachine::IsValidStateTransition(PLAYER_STATE _from, PLAYER_STATE _to) const
{
    // 같은 상태로의 전환은 허용하지 않음
    if (_from == _to)
        return false;

    // 특정 상태에서는 특정 상태로만 전환 가능
    switch (_from)
    {
    case PLAYER_STATE::SWALLOW:
        return false;

    case PLAYER_STATE::EXHALE:
        return false;

    case PLAYER_STATE::SLIDE:
        return (_to == PLAYER_STATE::FALL ||
            _to == PLAYER_STATE::CROUCH ||
            _to == PLAYER_STATE::IDLE);

    case PLAYER_STATE::HOVER:
        return (_to == PLAYER_STATE::HOVER_EXHALE);

        // HOVER_EXHALE에서의 제한 완전 제거
    case PLAYER_STATE::HOVER_EXHALE:
        // 모든 전환 허용 - 전환 테이블에서 조건 관리
        return true;
    }

    // 기본적으로 모든 전환 허용
    return true;
}