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
#include "CProjectileFactory.h"

CPlayerStateMachine::CPlayerStateMachine ( CPlayer* _pOwner )
    : m_pOwner ( _pOwner )
    , m_pInputManager ( nullptr )
    , m_pTransitionTable ( nullptr )
    , m_eCurState ( PLAYER_STATE::IDLE )
    , m_ePrevState ( PLAYER_STATE::END )
    , m_fFallTime ( 0.0f )
    , m_fFall0Duration ( 0.3f )
    , m_fFallStartY ( 0.0f )
    , m_fFallDistanceThreshold ( 224.0f )
    , m_fBounceHeight ( 0.6f )
    , m_bWasGrounded ( true )
    , m_fSlideTimer ( 0.0f )
    , m_fSlideDuration ( 0.8f )
    , m_fSlideDistance ( 256.f )
    , m_fSlideSpeed ( 320.f )
    , m_vSlideStartPos ( Vec2 ( 0.f , 0.f ) )
    , m_iSlideDirection ( 1 )
    , m_bSlideGroundCheck ( true )
    , m_bSlideKickCreated ( false )
    , m_fRecoilTimer ( 0.0f )
    , m_fRecoilDuration ( 0.3f )
    , m_vRecoilVelocity ( Vec2 ( 0.f , 0.f ) )
    , m_eHoverSubState ( HOVER_SUBSTATE::END )
    , m_fHoverUpForce ( 300.f )            // 상승력 조금 줄임
    , m_fHoverFallSpeed ( 80.f )           // 천천히 낙하하는 속도
    , m_fHoverMoveSpeed ( 120.f )          // 걷기 속도 정도
    , m_fHoverSubStateTimer ( 0.0f )
    , m_fHoverEnterDuration ( 0.6f )       // ENTER 지속 시간 늘림
    , m_fHoverFlyUpDuration ( 0.4f )       // FLY_UP 지속 시간 늘림
    , m_fHoverAirFriction ( 5.0f )         // 공중 마찰 계수
    , m_bHoverCanMoveOnGround ( true )     // 땅에서도 이동 가능
    , m_fDamageTimer ( 0.0f )              // 피격 타이머
    , m_fDamageDuration ( 0.5f )           // 0.5초 피격 상태
    , m_bDamageCompleted ( false )
    , m_eInhaleCount ( INHALE_COUNT::NONE )
    , m_eCopyAbility ( COPY_ABILITY::NONE )
    , m_fInhaleTimer ( 0.0f )
    , m_fInhaleDuration ( 1.0f )
    , m_fInhaleSuccessTimer ( 0.0f )
    , m_fInhaleSuccessDuration ( 0.3f )
    , m_fExhaleTimer ( 0.0f )
    , m_fExhaleDuration ( 0.5f )
    , m_fSwallowTimer ( 0.0f )
    , m_fSwallowDuration ( 0.8f )
{
    // 새로운 시스템들 생성
    m_pInputManager = new CPlayerInputManager ( );
    m_pTransitionTable = new CPlayerStateTransitionTable ( );
}

CPlayerStateMachine::~CPlayerStateMachine ( )
{
    if ( m_pInputManager )
    {
        delete m_pInputManager;
        m_pInputManager = nullptr;
    }

    if ( m_pTransitionTable )
    {
        delete m_pTransitionTable;
        m_pTransitionTable = nullptr;
    }
}

void CPlayerStateMachine::Init ( )
{
    // Movement 시스템에 InputManager 연결
    if ( m_pOwner && m_pOwner->GetMovement ( ) && m_pInputManager )
    {
        m_pOwner->GetMovement ( )->SetInputManager ( m_pInputManager );
    }

    // 전환 테이블 초기화
    InitializeTransitionTable ( );

    // 초기 상태 설정
    ChangeStateInternal ( PLAYER_STATE::IDLE );
}

void CPlayerStateMachine::Update ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody || !m_pInputManager || !m_pTransitionTable )
        return;

    // 1. 입력 수집
    m_pInputManager->Update ( );
    CPlayerInputManager::InputFlags currentInput = m_pInputManager->GetCurrentFrameInput ( );

    // 2. 상태 전환 체크
    PLAYER_STATE nextState = m_pTransitionTable->GetNextState ( m_eCurState , currentInput , m_pOwner );

    // 3. 상태 변경
    if ( nextState != m_eCurState )
    {
        ChangeStateInternal ( nextState );
    }

    // 4. 현재 상태 실행 (입력 처리 없음, 순수 실행만)
    ExecuteCurrentState ( );

    // 5. 이전 프레임 Ground 상태 업데이트
    m_bWasGrounded = pRigidBody->IsGround ( );
}

void CPlayerStateMachine::ChangeStateInternal ( PLAYER_STATE _eState )
{
    // 유효성 검사
    if ( !CanChangeToState ( _eState ) )
        return;

    // 실제 상태 변경
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;

    OnStateEnter ( _eState );

    // 애니메이션 설정
    SetAnimationForState ( _eState );
}

void CPlayerStateMachine::ExecuteCurrentState ( )
{
    switch ( m_eCurState )
    {
    case PLAYER_STATE::IDLE:
        ExecuteIdleState ( );
        break;
    case PLAYER_STATE::WALK:
    case PLAYER_STATE::RUN:
    case PLAYER_STATE::MOUTHFUL_IDLE:
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
        ExecuteMovementState ( );
        break;
    case PLAYER_STATE::JUMP:
    case PLAYER_STATE::MOUTHFUL_JUMP:
        ExecuteJumpState ( );
        break;
    case PLAYER_STATE::FALL0:
    case PLAYER_STATE::FALL1:
    case PLAYER_STATE::FALL2:
    case PLAYER_STATE::MOUTHFUL_FALL:
        ExecuteFallState ( );
        break;
    case PLAYER_STATE::CROUCH:
        ExecuteCrouchState ( );
        break;
    case PLAYER_STATE::HOVER:
        ExecuteHoverState ( );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        m_fHoverSubStateTimer += CTimeMgr::GetInst ( )->GetfDT ( );
        ExecuteHoverExhaleState ( );
        break;
    case PLAYER_STATE::SLIDE:
        ExecuteSlideState ( );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        ExecuteSlideKickRecoilState ( );
        break;
    case PLAYER_STATE::DAMAGE:
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        ExecuteDamageState ( );
        break;
    case PLAYER_STATE::INHALE:
        ExecuteInhaleState ( );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        ExecuteInhaleSuccessState ( );
        break;
    case PLAYER_STATE::EXHALE:
        ExecuteExhaleState ( );
        break;
    case PLAYER_STATE::SWALLOW:
        ExecuteSwallowState ( );
        break;
    case PLAYER_STATE::BOUNCE:
        ExecuteBounceState ( );
        break;
    }
}

void CPlayerStateMachine::ExecuteIdleState ( )
{
    // IDLE 상태에서는 특별한 처리 없음
    // 이동은 Movement 시스템에서, 상태 전환은 TransitionTable에서 처리
}

void CPlayerStateMachine::ExecuteMovementState ( )
{
    // 이동 처리는 CPlayerMovement에서 담당
    // 여기서는 상태 관련 물리만 처리
}

void CPlayerStateMachine::ExecuteJumpState ( )
{
    // 점프 높이 조절
    if ( m_pInputManager && m_pInputManager->IsJumpHold ( ) )
    {
        float jumpRatio = m_pInputManager->GetJumpHoldRatio ( );
        // 점프 높이 조절 로직 (필요시 구현)
    }
}

void CPlayerStateMachine::ExecuteFallState ( )
{
    // 낙하 시간 누적
    m_fFallTime += CTimeMgr::GetInst ( )->GetfDT ( );
}

void CPlayerStateMachine::ExecuteCrouchState ( )
{
    // 크라우치 상태에서 방향 전환 및 감속 처리
    if ( m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement()->HandleCrouchDirectionInput();
    }
}

void CPlayerStateMachine::ExecuteSlideState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    // 슬라이드 타이머 업데이트
    m_fSlideTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 슬라이드 킥 투사체 생성 (첫 프레임에만)
    UpdateSlideKickProjectile ( );

    // 슬라이드 이동 처리
    UpdateSlideMovement ( );

    // 슬라이드 완료 체크 (속도 정리만)
    CheckSlideCompletion ( );
}

void CPlayerStateMachine::ExecuteInhaleState ( )
{
    // 빨아들이기 상태 타이머 업데이트
    m_fInhaleTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // X키가 눌려있지 않고, 빨아들려지고 있는 몬스터가 없으면 종료
    if ( m_pInputManager && !m_pInputManager->IsActionHold ( ) )
    {
        // 빨아들이기 시스템에서 현재 빨아들이고 있는 대상이 있는지 확인
        bool hasInhaleTargets = false;
        if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
        {
            hasInhaleTargets = !m_pOwner->GetInhaleSystem ( )->GetInhaleTargets ( ).empty ( );
        }

        // 빨아들이고 있는 대상이 없으면 종료
        if ( !hasInhaleTargets )
        {
            // 땅에 있으면 IDLE, 공중이면 FALL1로
            if ( m_pOwner && m_pOwner->GetRigidBody ( ) && m_pOwner->GetRigidBody ( )->IsGround ( ) )
            {
                ChangeStateInternal ( PLAYER_STATE::IDLE );
            }
            else
            {
                ChangeStateInternal ( PLAYER_STATE::FALL1 );
            }
            return;
        }
    }

    // 빨아들이기 상태 시간 초과시 종료
    if ( m_fInhaleTimer >= m_fInhaleDuration )
    {
        // 빨아들이기 실패 후 기본 상태로 복귀
        ChangeStateInternal ( PLAYER_STATE::IDLE );
    }
}

void CPlayerStateMachine::ExecuteInhaleSuccessState ( )
{
    // 타이머 업데이트 (상태 전환 테이블에서 사용)
    m_fInhaleSuccessTimer += CTimeMgr::GetInst ( )->GetfDT ( );
    
    // 전환은 상태 전환 테이블에서 처리됨 (애니메이션 완료 또는 타이머 기반)
}

void CPlayerStateMachine::ExecuteExhaleState ( )
{
    // 내뱉기 상태 타이머 업데이트
    m_fExhaleTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 내뱉기 상태 시간 초과시 종료
    if ( m_fExhaleTimer >= m_fExhaleDuration )
    {
        // 빨아들이기 카운트 초기화
        m_eInhaleCount = INHALE_COUNT::NONE;
        m_eCopyAbility = COPY_ABILITY::NONE;

        // 기본 상태로 복귀
        ChangeStateInternal ( PLAYER_STATE::IDLE );
    }
}

void CPlayerStateMachine::ExecuteSwallowState ( )
{
    // 삼키기 상태 타이머 업데이트
    m_fSwallowTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 삼키기 상태 시간 초과시 종료
    if ( m_fSwallowTimer >= m_fSwallowDuration )
    {
        // 카피 능력 획득 (현재 COPY_ABILITY 유지)
        // 빨아들이기 카운트 초기화
        m_eInhaleCount = INHALE_COUNT::NONE;

        // 기본 상태로 복귀
        ChangeStateInternal ( PLAYER_STATE::IDLE );
    }
}

void CPlayerStateMachine::ExecuteBounceState ( )
{
}

void CPlayerStateMachine::ExecuteHoverState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody || !m_pInputManager )
        return;

    // 서브스테이트 타이머 업데이트
    m_fHoverSubStateTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 현재 서브스테이트에 따른 처리
    switch ( m_eHoverSubState )
    {
    case HOVER_SUBSTATE::ENTER:
        UpdateHoverEnter ( );
        break;
    case HOVER_SUBSTATE::FLY_UP:
        UpdateHoverFlyUp ( );
        break;
    case HOVER_SUBSTATE::FLOAT:
        UpdateHoverFloat ( );
        break;
    case HOVER_SUBSTATE::GROUNDED:
        UpdateHoverGrounded ( );
        break;
    }

    // 공통 처리
    UpdateHoverMovement ( );
    UpdateHoverPhysics ( );
}

void CPlayerStateMachine::ExecuteHoverExhaleState ( )
{
    // 정확히 한 번만 투사체 생성 (상태 진입 첫 프레임에만)
    static bool bProjectileFired = false;
    static float lastTimer = -1.0f;

    // 타이머가 리셋되었을 때 (새로운 HOVER_EXHALE 상태 진입)
    if ( m_fHoverSubStateTimer < lastTimer )
    {
        bProjectileFired = false;
    }
    lastTimer = m_fHoverSubStateTimer;

    // 아직 발사하지 않았고, 타이머가 매우 작을 때만 발사
    if ( !bProjectileFired && m_fHoverSubStateTimer <= 0.02f )
    {
        // 플레이어 위치와 방향 가져오기
        Vec2 vPlayerPos = m_pOwner->GetPos ( );
        Vec2 vDirection;

        // 플레이어가 보고 있는 방향 확인 (CPlayerMovement의 IsFacingRight 사용)
        CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
        if ( pMovement && !pMovement->IsFacingRight ( ) )
        {
            vDirection = Vec2 ( -1.f , 0.f );  // 왼쪽
            vPlayerPos.x -= 32.f;  // 투사체 시작 위치 조정
        }
        else
        {
            vDirection = Vec2 ( 1.f , 0.f );   // 오른쪽  
            vPlayerPos.x += 32.f;  // 투사체 시작 위치 조정
        }

        // 공기 투사체 생성
        CProjectile* pAirPuff = CProjectileFactory::CreateAirPuff (
            vPlayerPos ,
            vDirection ,
            GROUP_TYPE::PLAYER
        );

        if ( pAirPuff )
        {
            // 씬에 추가
            CREATE_OBJECT ( pAirPuff , GROUP_TYPE::PROJ_PLAYER );
            bProjectileFired = true;  // 발사했음을 표시
        }
    }
}

void CPlayerStateMachine::ExecuteDamageState ( )
{
    // 피격 타이머 업데이트
    m_fDamageTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 피격 시간이 끝나면 완료 플래그 설정 (전환은 테이블에서)
    if ( m_fDamageTimer >= m_fDamageDuration )
    {
        m_bDamageCompleted = true;
    }
}

void CPlayerStateMachine::ExecuteSlideKickRecoilState ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    // 반동 타이머 업데이트
    m_fRecoilTimer += CTimeMgr::GetInst ( )->GetfDT ( );

    // 반동 물리 처리 (감속 적용)
    float decayFactor = 1.0f - ( m_fRecoilTimer / m_fRecoilDuration );
    if ( decayFactor < 0.0f ) decayFactor = 0.0f;

    Vec2 currentVelocity = m_vRecoilVelocity * decayFactor;
    pRigidBody->SetVelocity ( currentVelocity );
}

void CPlayerStateMachine::OnStateEnter ( PLAYER_STATE _eState )
{
    // 이전 상태가 INHALE이었다면 빨아들이기 시스템 중지
    if ( m_ePrevState == PLAYER_STATE::INHALE && _eState != PLAYER_STATE::INHALE )
    {
        if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
        {
            m_pOwner->GetInhaleSystem ( )->StopInhale ( );
        }
    }

    switch ( _eState )
    {
    case PLAYER_STATE::JUMP:
        OnEnterJumpState ( );
        break;
    case PLAYER_STATE::SLIDE:
        OnEnterSlideState ( );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        OnEnterSlideKickRecoilState ( );
        break;
    case PLAYER_STATE::INHALE:
        OnEnterInhaleState ( );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        OnEnterInhaleSuccessState ( );
        break;
    case PLAYER_STATE::EXHALE:
        OnEnterExhaleState ( );
        break;
    case PLAYER_STATE::SWALLOW:
        OnEnterSwallowState ( );
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
    case PLAYER_STATE::MOUTHFUL_WALK:
    case PLAYER_STATE::MOUTHFUL_RUN:
    case PLAYER_STATE::MOUTHFUL_JUMP:
    case PLAYER_STATE::MOUTHFUL_FALL:
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        OnEnterMouthfulState ( );
        break;
    case PLAYER_STATE::BOUNCE:
        OnEnterBounceState ( );
        break;
    case PLAYER_STATE::FALL0:
    case PLAYER_STATE::FALL1:
        OnEnterFallState ( );
        break;
    case PLAYER_STATE::HOVER:
        OnEnterHoverState ( );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        OnEnterHoverExhaleState ( );
        break;
    case PLAYER_STATE::DAMAGE:
        OnEnterDamageState ( );
        break;
    }
}

void CPlayerStateMachine::OnEnterJumpState ( )
{
    if ( m_pOwner && m_pOwner->GetMovement ( ) )
    {
        m_pOwner->GetMovement ( )->Jump ( );
    }
}

void CPlayerStateMachine::OnEnterSlideState ( )
{
    InitiateSlide ( );
}

void CPlayerStateMachine::OnEnterSlideKickRecoilState ( )
{
    m_fRecoilTimer = 0.0f;

    // 반동 요청 플래그 클리어
    if ( m_pOwner )
    {
        m_pOwner->ClearSlideKickRecoilRequest ( );
    }

    // 반동 속도 설정 (반대방향 대각 위로)
    float recoilX = -m_iSlideDirection * 200.0f;
    float recoilY = -200.0f;

    m_vRecoilVelocity = Vec2 ( recoilX , recoilY );

    if ( m_pOwner && m_pOwner->GetRigidBody ( ) )
    {
        m_pOwner->GetRigidBody ( )->SetVelocity ( m_vRecoilVelocity );
        m_pOwner->GetRigidBody ( )->SetUseGravity ( true );
    }
}

void CPlayerStateMachine::OnEnterInhaleState ( )
{
    // 빨아들이기 타이머 초기화
    m_fInhaleTimer = 0.0f;

    // 빨아들이기 시스템 시작
    if ( m_pOwner && m_pOwner->GetInhaleSystem ( ) )
    {
        m_pOwner->GetInhaleSystem ( )->StartInhale ( );
    }
}

void CPlayerStateMachine::OnEnterInhaleSuccessState ( )
{
    // 빨아들이기 성공 타이머 초기화
    m_fInhaleSuccessTimer = 0.0f;
}

void CPlayerStateMachine::OnEnterExhaleState ( )
{
    // 내뱉기 타이머 초기화
    m_fExhaleTimer = 0.0f;

    // 투사체 생성 (빨아들인 것이 있을 경우에만)
    if ( m_pOwner && m_eInhaleCount != INHALE_COUNT::NONE )
    {
        // TODO: CProjectileFactory를 통한 투사체 생성
        // KIRBY_STAR 또는 KIRBY_STAR_ENHANCED 타입 투사체 생성
    }
}

void CPlayerStateMachine::OnEnterSwallowState ( )
{
    // 삼키기 타이머 초기화
    m_fSwallowTimer = 0.0f;
}

void CPlayerStateMachine::OnEnterMouthfulState ( )
{
    // 입가득한 상태 진입 처리
}

void CPlayerStateMachine::OnEnterBounceState ( )
{
    PerformBounce ( );
}

void CPlayerStateMachine::OnEnterFallState ( )
{
    // FALL0/FALL1 진입 시: 낙하 시작 높이 기록 + 타이머 초기화
    if ( m_pOwner )
    {
        m_fFallStartY = m_pOwner->GetPos ( ).y;
        m_fFallTime = 0.f;
    }
}

void CPlayerStateMachine::OnEnterHoverState ( )
{
    // HOVER 상태 진입 시 초기화
    m_eHoverSubState = HOVER_SUBSTATE::ENTER;
    ResetHoverSubStateTimer ( );

    // 중력 비활성화
    DisableGravityForHover ( );
}

void CPlayerStateMachine::OnEnterHoverExhaleState ( )
{
    // 타이머 초기화
    m_fHoverSubStateTimer = 0.0f;

    // HOVER 종료 시 중력 재활성화
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( true );
    }
}

void CPlayerStateMachine::OnEnterDamageState ( )
{
    m_fDamageTimer = 0.0f;
    m_bDamageCompleted = false;

    // 피격 플래그 정리
    if ( m_pOwner )
    {
        m_pOwner->ClearDamageRequest ( );
    }
}

void CPlayerStateMachine::InitializeTransitionTable ( )
{
    if ( !m_pTransitionTable )
        return;

    // 기본 전환들 추가
    AddBasicMovementTransitions ( );
    AddJumpAndFallTransitions ( );
    AddCrouchAndSlideTransitions ( );
    AddInhaleTransitions ( );
    AddSpecialTransitions ( );
    AddHoverTransitions ( );
    AddDamageTransitions ( );
}

void CPlayerStateMachine::AddBasicMovementTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;
    
    // 공통 조건 함수들
    auto IsGrounded = [] ( CPlayer* p ) {
        return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
    };
    
    auto IsStoppedCompletely = [] ( CPlayer* p ) {
        if ( !p->GetMovement ( ) ) return false;
        CPlayerInputManager* pInputMgr = p->GetStateMachine ( )->GetInputManager ( );
        if ( !pInputMgr ) return false;
        
        bool hasLeftInput = pInputMgr->IsMovingLeft ( );
        bool hasRightInput = pInputMgr->IsMovingRight ( );
        bool isMoving = p->GetMovement ( )->IsActuallyMoving ( );
        bool isDecelerating = p->GetMovement ( )->IsDecelerating ( );
        
        return !hasLeftInput && !hasRightInput && !isMoving && !isDecelerating;
    };

    // IDLE <-> WALK 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::IDLE ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::WALK ,
        IsGrounded ,
        100
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::WALK ,
        0 ,
        PLAYER_STATE::IDLE ,
        IsStoppedCompletely ,
        50 ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT
    );

    // 더블탭 RUN 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::WALK ,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT | ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT ,
        PLAYER_STATE::RUN ,
        IsGrounded ,
        150
    );

    // RUN -> WALK
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::RUN ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::WALK ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsRunMode ( );
        } ,
        80
    );

    // RUN -> IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::RUN ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsActuallyMoving ( ) &&
                !p->GetMovement ( )->IsDecelerating ( );
        } ,
        60
    );
}

void CPlayerStateMachine::AddJumpAndFallTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;
    
    // 공통 조건 함수들
    auto IsGrounded = [] ( CPlayer* p ) {
        return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
    };

    // 점프 시작
    std::vector<PLAYER_STATE> jumpableStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN
    };

    for ( PLAYER_STATE state : jumpableStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::JUMP ,
            IsGrounded ,
            200
        );
    }

    // 점프 -> FALL0
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::JUMP ,
        0 ,
        PLAYER_STATE::FALL0 ,
        [ this ] ( CPlayer* p ) {
            if ( !p->GetRigidBody ( ) ) return false;
            return p->GetRigidBody ( )->GetVelocity ( ).y > 50.f;
        } ,
        300
    );

    // FALL0 -> FALL1 (0.3초 후)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::FALL0 ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fFallTime >= m_fFall0Duration;
        } ,
        350
    );

    // FALL0/FALL1 -> 착지
    std::vector<PLAYER_STATE> fallStates = { PLAYER_STATE::FALL0, PLAYER_STATE::FALL1 };
    for ( PLAYER_STATE state : fallStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::IDLE ,
            [ this ] ( CPlayer* p ) {
                if ( !p->GetRigidBody ( ) ) return false;
                return p->GetRigidBody ( )->IsGround ( ) && !m_bWasGrounded;
            } ,
            300
        );
    }

    // FALL0/FALL1 -> FALL2 (거리 기반)
    for ( PLAYER_STATE state : fallStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::FALL2 ,
            [ this ] ( CPlayer* p ) {
                if ( !p ) return false;
                float currentY = p->GetPos ( ).y;
                float fallDistance = currentY - m_fFallStartY;
                return fallDistance >= m_fFallDistanceThreshold;
            } ,
            330
        );
    }

    // FALL2 -> BOUNCE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::FALL2 ,
        0 ,
        PLAYER_STATE::BOUNCE ,
        [ ] ( CPlayer* p ) { return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ); } ,
        350
    );

    // 지상 -> 낙하
    std::vector<PLAYER_STATE> groundStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN, PLAYER_STATE::CROUCH
    };

    for ( PLAYER_STATE state : groundStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::FALL1 ,
            [ ] ( CPlayer* p ) {
                CRigidBody* pRB = p->GetRigidBody ( );
                return pRB && !pRB->IsGround ( );
            } ,
            500
        );
    }
}

void CPlayerStateMachine::AddCrouchAndSlideTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 크라우치 진입
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::IDLE ,
        ( uint32_t ) INPUT::MOVE_DOWN ,
        PLAYER_STATE::CROUCH ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ) &&
                !p->GetStateMachine ( )->IsMouthfulState ( );
        } ,
        150
    );

    // 크라우치 해제
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::CROUCH ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) {
            if ( !m_pInputManager ) return false;
            return !m_pInputManager->IsMovingDown ( );
        } ,
        100
    );

    // 슬라이드 시작
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::CROUCH ,
        ( uint32_t ) INPUT::JUMP_TAP | ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::SLIDE ,
        [ ] ( CPlayer* p ) { return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( ); } ,
        250
    );

    // 슬라이드킥 반동 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::SLIDE_KICK_RECOIL ,
        [ ] ( CPlayer* p ) {
            return p && p->IsSlideKickRecoilRequested ( );
        } ,
        500
    );

    // 반동 완료 -> FALL1
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE_KICK_RECOIL ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fRecoilTimer >= m_fRecoilDuration ||
                ( p->GetRigidBody ( ) && p->GetRigidBody ( )->GetVelocity ( ).Length ( ) < 50.0f );
        } ,
        400
    );

    // 슬라이드 완료 전환들
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::CROUCH ,
        [ this ] ( CPlayer* p ) {
            if ( !IsSlideCompleted ( ) ) return false;
            return m_pInputManager && m_pInputManager->IsMovingDown ( );
        } ,
        280
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ this ] ( CPlayer* p ) {
            if ( !IsSlideCompleted ( ) ) return false;
            return !m_pInputManager || !m_pInputManager->IsMovingDown ( );
        } ,
        270
    );

    // 슬라이드 -> 낙하
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SLIDE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            return pRB && !pRB->IsGround ( ) && m_bSlideGroundCheck;
        } ,
        400
    );
}

void CPlayerStateMachine::AddInhaleTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 빨아들이기 가능한 상태들에서 X키 누르면 INHALE로 전환
    std::vector<PLAYER_STATE> inhalableStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::JUMP, 
        PLAYER_STATE::FALL0, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2,
        PLAYER_STATE::BOUNCE
    };

    for ( PLAYER_STATE state : inhalableStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::ACTION_HOLD ,
            PLAYER_STATE::INHALE ,
            nullptr ,
            200
        );
    }

    // INHALE 상태 유지 (X키 계속 누르고 있으면)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_HOLD ,
        PLAYER_STATE::INHALE ,
        nullptr ,
        100
    );

    // INHALE -> INHALE_SUCCESS
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        0 ,
        PLAYER_STATE::INHALE_SUCCESS ,
        [ ] ( CPlayer* p ) {
            // TODO: 빨아들이기 시스템에서 성공 상태 체크
            // 임시로 StateMachine의 InhaleCount를 체크
            CPlayerStateMachine* pSM = p->GetStateMachine ( );
            return pSM && pSM->GetInhaleCount ( ) != INHALE_COUNT::NONE;
        } ,
        300
    );

    // INHALE -> 원래 상태로 복귀 (X키 떼면)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            // 땅에 있으면 IDLE로
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        250
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE ,
        ( uint32_t ) INPUT::ACTION_AWAY ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) {
            // 공중에 있으면 FALL1로
            return p->GetRigidBody ( ) && !p->GetRigidBody ( )->IsGround ( );
        } ,
        250
    );

    // INHALE_SUCCESS -> MOUTHFUL_IDLE (애니메이션 완료 시 자동 전환)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::INHALE_SUCCESS ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ this ] ( CPlayer* p ) {
            // 애니메이션이 끝났거나 시간 경과시 자동 전환
            CAnimator* pAnimator = p->GetAnimator();
            bool bAnimFinished = false;
            if (pAnimator && pAnimator->GetCurAnim())
            {
                bAnimFinished = pAnimator->GetCurAnim()->IsFinish();
            }
            bool bTimeElapsed = m_fInhaleSuccessTimer >= m_fInhaleSuccessDuration;
            return bAnimFinished || bTimeElapsed;
        } ,
        400
    );

    // MOUTHFUL_IDLE -> EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::EXHALE ,
        nullptr ,
        300
    );

    // EXHALE -> IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::EXHALE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            return true;
        } ,
        400
    );

    // SWALLOW -> IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SWALLOW ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            return true;
        } ,
        400
    );
}

void CPlayerStateMachine::AddSpecialTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 애니메이션 기반 전환들
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::EXHALE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnimator = p->GetAnimator ( );
            if ( !pAnimator ) return false;
            CAnimation* pCurAnim = pAnimator->GetCurAnim ( );
            return pCurAnim && pCurAnim->IsFinish ( );
        } ,
        400
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::SWALLOW ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnimator = p->GetAnimator ( );
            if ( !pAnimator ) return false;
            CAnimation* pCurAnim = pAnimator->GetCurAnim ( );
            return pCurAnim && pCurAnim->IsFinish ( );
        } ,
        400
    );

    // BOUNCE -> FALL
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::BOUNCE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            if ( !pRB ) return false;
            Vec2 vVelocity = pRB->GetVelocity ( );
            bool isGrounded = pRB->IsGround ( );
            return !isGrounded && vVelocity.y >= 0.f;
        } ,
        350
    );

    // 입가득한 상태들 간의 전환
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_IDLE ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::MOUTHFUL_WALK ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        100
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_WALK ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            CPlayerInputManager* pInputMgr = p->GetStateMachine ( )->GetInputManager ( );
            if ( !pInputMgr ) return false;
            return !pInputMgr->IsMovingLeft ( ) && !pInputMgr->IsMovingRight ( );
        } ,
        50
    );

    // MOUTHFUL_WALK -> MOUTHFUL_RUN (더블탭으로 달리기)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_WALK ,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT | ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT ,
        PLAYER_STATE::MOUTHFUL_RUN ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        120
    );

    // MOUTHFUL_RUN -> MOUTHFUL_WALK (달리기 해제)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_RUN ,
        ( uint32_t ) INPUT::MOVE_LEFT | ( uint32_t ) INPUT::MOVE_RIGHT ,
        PLAYER_STATE::MOUTHFUL_WALK ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsRunMode ( );
        } ,
        100
    );

    // MOUTHFUL_RUN -> MOUTHFUL_IDLE (정지)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_RUN ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            if ( !p->GetMovement ( ) ) return false;
            return !p->GetMovement ( )->IsActuallyMoving ( ) &&
                !p->GetMovement ( )->IsDecelerating ( );
        } ,
        60
    );

    // MOUTHFUL 점프 전환들
    std::vector<PLAYER_STATE> mouthfulGroundStates = {
        PLAYER_STATE::MOUTHFUL_IDLE, PLAYER_STATE::MOUTHFUL_WALK, PLAYER_STATE::MOUTHFUL_RUN
    };

    for ( PLAYER_STATE state : mouthfulGroundStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::MOUTHFUL_JUMP ,
            [ ] ( CPlayer* p ) {
                return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
            } ,
            200
        );
    }

    // MOUTHFUL_JUMP -> MOUTHFUL_FALL
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_JUMP ,
        0 ,
        PLAYER_STATE::MOUTHFUL_FALL ,
        [ ] ( CPlayer* p ) {
            CRigidBody* pRB = p->GetRigidBody ( );
            return pRB && !pRB->IsGround ( ) && pRB->GetVelocity ( ).y >= 0.f;
        } ,
        150
    );

    // MOUTHFUL_FALL -> MOUTHFUL_IDLE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_FALL ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) {
            return p->GetRigidBody ( ) && p->GetRigidBody ( )->IsGround ( );
        } ,
        180
    );
}

void CPlayerStateMachine::AddHoverTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // HOVER 진입 전환
    std::vector<PLAYER_STATE> hoverableStates = {
        PLAYER_STATE::JUMP, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2
    };

    for ( PLAYER_STATE state : hoverableStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            ( uint32_t ) INPUT::JUMP_TAP ,
            PLAYER_STATE::HOVER ,
            [ ] ( CPlayer* p ) {
                return p->GetRigidBody ( ) && !p->GetRigidBody ( )->IsGround ( );
            } ,
            250
        );
    }

    // HOVER -> HOVER_EXHALE
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER ,
        ( uint32_t ) INPUT::ACTION_TAP ,
        PLAYER_STATE::HOVER_EXHALE ,
        nullptr ,
        400
    );

    // HOVER_EXHALE 종료 전환들
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnim = p->GetAnimator ( );
            CRigidBody* pRigidBody = p->GetRigidBody ( );

            if ( !pAnim || !pRigidBody ) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim ( );
            if ( !pCurAnim ) return false;

            bool animFinished = pCurAnim->IsFinish ( );
            bool lastFrameReached = ( pCurAnim->GetCurFrame ( ) >= pCurAnim->GetMaxFrame ( ) - 1 );
            bool isGrounded = pRigidBody->IsGround ( );

            return ( animFinished || lastFrameReached ) && isGrounded;
        } ,
        350
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) {
            CAnimator* pAnim = p->GetAnimator ( );
            CRigidBody* pRigidBody = p->GetRigidBody ( );

            if ( !pAnim || !pRigidBody ) return false;

            CAnimation* pCurAnim = pAnim->GetCurAnim ( );
            if ( !pCurAnim ) return false;

            bool animFinished = pCurAnim->IsFinish ( );
            bool lastFrameReached = ( pCurAnim->GetCurFrame ( ) >= pCurAnim->GetMaxFrame ( ) - 1 );
            bool isAirborne = !pRigidBody->IsGround ( );

            return ( animFinished || lastFrameReached ) && isAirborne;
        } ,
        340
    );

    // 백업 전환 (시간 기반)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::HOVER_EXHALE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ this ] ( CPlayer* p ) {
            return m_fHoverSubStateTimer > 1.0f;
        } ,
        100
    );
}

void CPlayerStateMachine::AddDamageTransitions ( )
{
    using INPUT = CPlayerInputManager::INPUT_TYPE;

    // 피격 조건 정의
    auto damageCondition = [ ] ( CPlayer* p ) -> bool {
        if ( !p ) return false;
        if ( p->GetHealthSystem ( ) && p->GetHealthSystem ( )->IsGameOver ( ) )
            return false;
        return p->IsDamageRequested ( );
        };

    // 모든 상태에서 DAMAGE로의 전환
    std::vector<PLAYER_STATE> allStates = {
        PLAYER_STATE::IDLE, PLAYER_STATE::WALK, PLAYER_STATE::RUN,
        PLAYER_STATE::JUMP, PLAYER_STATE::FALL1, PLAYER_STATE::FALL2,
        PLAYER_STATE::CROUCH, PLAYER_STATE::SLIDE, PLAYER_STATE::HOVER,
        PLAYER_STATE::INHALE, PLAYER_STATE::INHALE_SUCCESS, PLAYER_STATE::EXHALE, PLAYER_STATE::SWALLOW
    };

    for ( PLAYER_STATE state : allStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::DAMAGE ,
            damageCondition ,
            2000
        );
    }

    // MOUTHFUL 상태들에서 MOUTHFUL_DAMAGE로의 전환
    std::vector<PLAYER_STATE> mouthfulStates = {
        PLAYER_STATE::MOUTHFUL_IDLE, PLAYER_STATE::MOUTHFUL_WALK, PLAYER_STATE::MOUTHFUL_RUN,
        PLAYER_STATE::MOUTHFUL_JUMP, PLAYER_STATE::MOUTHFUL_FALL
    };

    for ( PLAYER_STATE state : mouthfulStates )
    {
        m_pTransitionTable->AddTransition (
            state ,
            0 ,
            PLAYER_STATE::MOUTHFUL_DAMAGE ,
            damageCondition ,
            2000
        );
    }

    // DAMAGE 상태에서의 전환 (완료 플래그 기반)
    m_pTransitionTable->AddTransition (
        PLAYER_STATE::DAMAGE ,
        0 ,
        PLAYER_STATE::IDLE ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::MOUTHFUL_DAMAGE ,
        0 ,
        PLAYER_STATE::MOUTHFUL_IDLE ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    m_pTransitionTable->AddTransition (
        PLAYER_STATE::DAMAGE ,
        0 ,
        PLAYER_STATE::FALL1 ,
        [ ] ( CPlayer* p ) -> bool {
            if ( !p || !p->GetRigidBody ( ) || !p->GetStateMachine ( ) )
                return false;
            return p->GetStateMachine ( )->IsDamageCompleted ( ) &&
                !p->GetRigidBody ( )->IsGround ( );
        } ,
        1000
    );

    // 입력 무시 규칙들
    std::vector<uint32_t> ignoredInputs = {
        ( uint32_t ) INPUT::JUMP_TAP, ( uint32_t ) INPUT::ACTION_TAP,
        ( uint32_t ) INPUT::MOVE_LEFT, ( uint32_t ) INPUT::MOVE_RIGHT, ( uint32_t ) INPUT::MOVE_DOWN,
        ( uint32_t ) INPUT::DOUBLE_TAP_LEFT, ( uint32_t ) INPUT::DOUBLE_TAP_RIGHT
    };

    for ( uint32_t input : ignoredInputs )
    {
        m_pTransitionTable->AddTransition (
            PLAYER_STATE::DAMAGE ,
            input ,
            PLAYER_STATE::DAMAGE ,
            nullptr ,
            900
        );
    }
}

void CPlayerStateMachine::InitiateSlide ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody )
        return;

    m_fSlideTimer = 0.0f;
    m_vSlideStartPos = m_pOwner->GetPos ( );
    m_bSlideGroundCheck = true;
    m_bSlideKickCreated = false;

    CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
    if ( pMovement )
    {
        m_iSlideDirection = pMovement->IsFacingRight ( ) ? 1 : -1;
    }
    else
    {
        m_iSlideDirection = 1;
    }

    pRigidBody->SetVelocityX ( m_fSlideSpeed * m_iSlideDirection );
}

void CPlayerStateMachine::UpdateSlideMovement ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody )
        return;

    float slideProgress = m_fSlideTimer / m_fSlideDuration;

    if ( slideProgress <= 1.0f )
    {
        float speedMultiplier = 1.0f - ( slideProgress * slideProgress );
        float currentSlideSpeed = m_fSlideSpeed * speedMultiplier;
        currentSlideSpeed = max ( currentSlideSpeed , m_fSlideSpeed * 0.3f );

        pRigidBody->SetVelocityX ( currentSlideSpeed * m_iSlideDirection );
    }
}

void CPlayerStateMachine::CheckSlideCompletion ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    if ( IsSlideCompleted ( ) )
    {
        pRigidBody->SetVelocityX ( 0.f );
    }
}

bool CPlayerStateMachine::IsSlideCompleted ( ) const
{
    if ( m_eCurState != PLAYER_STATE::SLIDE ) return false;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return true;

    bool timeCompleted = ( m_fSlideTimer >= m_fSlideDuration );

    Vec2 currentPos = m_pOwner->GetPos ( );
    float distanceTraveled = abs ( currentPos.x - m_vSlideStartPos.x );
    bool distanceCompleted = ( distanceTraveled >= m_fSlideDistance );

    float currentSpeedX = abs ( pRigidBody->GetVelocity ( ).x );
    bool speedTooSlow = ( currentSpeedX < 50.f );

    return timeCompleted || distanceCompleted || speedTooSlow;
}

void CPlayerStateMachine::UpdateSlideKickProjectile ( )
{
    if ( m_bSlideKickCreated || !m_pOwner )
        return;

    if ( m_fSlideTimer <= 0.02f )
    {
        Vec2 vPlayerPos = m_pOwner->GetPos ( );
        Vec2 vDirection = Vec2 ( static_cast< float >( m_iSlideDirection ) , 0.f );

        Vec2 vKickPos = vPlayerPos;
        vKickPos.x += ( 28.f * m_iSlideDirection );
        vKickPos.y += 20.f;

        CProjectile* pSlideKick = CProjectileFactory::CreateSlideKick (
            vKickPos ,
            vDirection ,
            GROUP_TYPE::PLAYER
        );

        if ( pSlideKick )
        {
            CREATE_OBJECT ( pSlideKick , GROUP_TYPE::PROJ_PLAYER );
            m_bSlideKickCreated = true;
        }
    }
}

// 기존 함수들
bool CPlayerStateMachine::IsInhaleState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::INHALE ||
        m_eCurState == PLAYER_STATE::INHALE_SUCCESS );
}

bool CPlayerStateMachine::IsMovingState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::WALK ||
        m_eCurState == PLAYER_STATE::RUN ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_WALK ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_RUN );
}

bool CPlayerStateMachine::IsMouthfulState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::MOUTHFUL_IDLE ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_WALK ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_RUN ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_JUMP ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_FALL ||
        m_eCurState == PLAYER_STATE::MOUTHFUL_DAMAGE );
}

bool CPlayerStateMachine::IsGroundedState ( ) const
{
    return ( m_eCurState != PLAYER_STATE::JUMP &&
        m_eCurState != PLAYER_STATE::FALL0 &&
        m_eCurState != PLAYER_STATE::FALL1 &&
        m_eCurState != PLAYER_STATE::FALL2 &&
        m_eCurState != PLAYER_STATE::BOUNCE &&
        m_eCurState != PLAYER_STATE::MOUTHFUL_JUMP &&
        m_eCurState != PLAYER_STATE::MOUTHFUL_FALL &&
        m_eCurState != PLAYER_STATE::HOVER &&
        m_eCurState != PLAYER_STATE::HOVER_EXHALE );
}

bool CPlayerStateMachine::IsHoverState ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER ||
        m_eCurState == PLAYER_STATE::HOVER_EXHALE );
}

bool CPlayerStateMachine::IsHoverGrounded ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER &&
        m_eHoverSubState == HOVER_SUBSTATE::GROUNDED );
}

bool CPlayerStateMachine::IsHoverFloating ( ) const
{
    return ( m_eCurState == PLAYER_STATE::HOVER &&
        ( m_eHoverSubState == HOVER_SUBSTATE::FLOAT ||
            m_eHoverSubState == HOVER_SUBSTATE::FLY_UP ) );
}

void CPlayerStateMachine::ForceStateChange ( PLAYER_STATE _eState )
{
    if ( IsValidStateTransition ( m_eCurState , _eState ) )
    {
        ChangeStateInternal ( _eState );
    }
}

void CPlayerStateMachine::ForceStateForSystemReset ( PLAYER_STATE _eState )
{
    if ( _eState == PLAYER_STATE::IDLE ||
        _eState == PLAYER_STATE::END )
    {
        ChangeStateInternal ( _eState );

        m_fFallTime = 0.0f;
        m_fFallStartY = 0.0f;
        m_fSlideTimer = 0.0f;
        m_fDamageTimer = 0.0f;
        m_bDamageCompleted = false;
    }
}

bool CPlayerStateMachine::CanChangeToState ( PLAYER_STATE _eState ) const
{
    return IsValidStateTransition ( m_eCurState , _eState );
}

void CPlayerStateMachine::PerformBounce ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !m_pOwner || !pRigidBody )
        return;

    CPlayerMovement* pMovement = m_pOwner->GetMovement ( );
    if ( pMovement )
    {
        float baseJumpPower = pMovement->GetJumpPower ( );
        float bounceJumpPower = baseJumpPower * m_fBounceHeight;

        pRigidBody->SetVelocityY ( -bounceJumpPower );
        pRigidBody->SetGround ( false );
    }
}

// HOVER 관련 함수들
void CPlayerStateMachine::UpdateHoverEnter ( )
{
    if ( m_fHoverSubStateTimer < 0.05f )
    {
        ApplyHoverUpForce ( );
    }

    if ( m_fHoverSubStateTimer >= m_fHoverEnterDuration )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
    }
    else
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            Vec2 vVelocity = pRigidBody->GetVelocity ( );
            if ( vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.2f )
            {
                ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFlyUp ( )
{
    if ( m_fHoverSubStateTimer < 0.05f )
    {
        ApplyHoverUpForce ( );
    }

    if ( m_fHoverSubStateTimer >= m_fHoverFlyUpDuration )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
    }
    else
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            Vec2 vVelocity = pRigidBody->GetVelocity ( );
            if ( vVelocity.y >= -10.f && m_fHoverSubStateTimer > 0.1f )
            {
                ChangeHoverSubState ( HOVER_SUBSTATE::FLOAT );
            }
        }
    }
}

void CPlayerStateMachine::UpdateHoverFloat ( )
{
    if ( m_pInputManager->IsJumpTap ( ) )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::FLY_UP );
    }

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
    if ( pRigidBody && pRigidBody->IsGround ( ) )
    {
        ChangeHoverSubState ( HOVER_SUBSTATE::GROUNDED );
    }
}

void CPlayerStateMachine::UpdateHoverGrounded ( )
{
    if ( m_pInputManager->IsJumpTap ( ) )
    {
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody ( );
        if ( pRigidBody )
        {
            pRigidBody->SetGround ( false );
        }
        ChangeHoverSubState ( HOVER_SUBSTATE::FLY_UP );
    }
}

void CPlayerStateMachine::UpdateHoverMovement ( )
{
    if ( !m_pInputManager ) return;

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    // 땅에서 이동 불가 설정이면 GROUNDED 상태에서 이동 제한
    if ( !m_bHoverCanMoveOnGround && m_eHoverSubState == HOVER_SUBSTATE::GROUNDED )
        return;

    // 좌우 이동 입력 처리
    int horizontalInput = m_pInputManager->GetHorizontalInput ( );
    if ( horizontalInput != 0 )
    {
        float moveVelocity = m_fHoverMoveSpeed * horizontalInput;
        pRigidBody->SetVelocityX ( moveVelocity );

        // 방향 업데이트
        if ( m_pOwner->GetMovement ( ) )
        {
            m_pOwner->GetMovement ( )->SetFacingDirection ( horizontalInput > 0 );
        }
    }
    else
    {
        // 입력이 없으면 X축 속도 감속 (공중 마찰)
        Vec2 vVel = pRigidBody->GetVelocity ( );
        float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
        float newVelX = vVel.x * ( 1.0f - m_fHoverAirFriction * deltaTime );

        if ( abs ( newVelX ) < 10.f ) newVelX = 0.f;
        pRigidBody->SetVelocityX ( newVelX );
    }
}

void CPlayerStateMachine::UpdateHoverPhysics ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    // 서브스테이트에 따른 물리 처리
    switch ( m_eHoverSubState )
    {
    case HOVER_SUBSTATE::ENTER:
        // ENTER 상태에서는 초기 상승 후 서서히 감속
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f )
        {
            // 상승 속도를 서서히 감소시킴
            float deceleration = 600.f;
            float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if ( newVelY > 0 ) newVelY = 0;
            pRigidBody->SetVelocityY ( newVelY );
        }
    }
    break;

    case HOVER_SUBSTATE::FLY_UP:
        // FLY_UP 상태에서도 같은 처리
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y < 0 && m_fHoverSubStateTimer > 0.1f )
        {
            float deceleration = 600.f;
            float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
            float newVelY = vVelocity.y + deceleration * deltaTime;

            if ( newVelY > 0 ) newVelY = 0;
            pRigidBody->SetVelocityY ( newVelY );
        }
    }
    break;

    case HOVER_SUBSTATE::FLOAT:
        // FLOAT 상태에서는 천천히 낙하
        ApplyHoverFallSpeed ( );
        break;

    case HOVER_SUBSTATE::GROUNDED:
        // GROUNDED 상태에서는 낙하 없음 (땅에 붙어있음)
    {
        Vec2 vVelocity = pRigidBody->GetVelocity ( );
        if ( vVelocity.y > 0 )
        {
            pRigidBody->SetVelocityY ( 0 );
        }
    }
    break;
    }
}

void CPlayerStateMachine::ApplyHoverUpForce ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetVelocityY ( -m_fHoverUpForce );
    }
}

void CPlayerStateMachine::ApplyHoverFallSpeed ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( !pRigidBody ) return;

    Vec2 vVelocity = pRigidBody->GetVelocity ( );

    // 현재 Y 속도가 HOVER 낙하 속도보다 작으면 낙하 속도로 설정
    if ( vVelocity.y < m_fHoverFallSpeed )
    {
        // 서서히 낙하 속도까지 증가
        float deltaTime = CTimeMgr::GetInst ( )->GetfDT ( );
        float acceleration = 200.f;
        float newVelY = vVelocity.y + acceleration * deltaTime;

        // 최대 낙하 속도 제한
        if ( newVelY > m_fHoverFallSpeed )
        {
            newVelY = m_fHoverFallSpeed;
        }

        pRigidBody->SetVelocityY ( newVelY );
    }
}

void CPlayerStateMachine::DisableGravityForHover ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( false );
    }
}

void CPlayerStateMachine::RestoreGravityFromHover ( )
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody ( ) : nullptr;
    if ( pRigidBody )
    {
        pRigidBody->SetUseGravity ( true );
    }
}

void CPlayerStateMachine::ChangeHoverSubState ( HOVER_SUBSTATE _eNewSubState )
{
    if ( m_eHoverSubState == _eNewSubState )
        return;

    m_eHoverSubState = _eNewSubState;
    ResetHoverSubStateTimer ( );

    // 현재 HOVER 상태라면 애니메이션 재설정
    if ( m_eCurState == PLAYER_STATE::HOVER )
    {
        SetAnimationForState ( PLAYER_STATE::HOVER );
    }
}

void CPlayerStateMachine::SetAnimationForState ( PLAYER_STATE _eState )
{
    CAnimator* pAnimator = m_pOwner ? m_pOwner->GetAnimator ( ) : nullptr;
    if ( !pAnimator )
        return;

    switch ( _eState )
    {
    case PLAYER_STATE::IDLE:
        pAnimator->Play ( L"IDLE" , true );
        break;
    case PLAYER_STATE::WALK:
        pAnimator->Play ( L"WALK" , true );
        break;
    case PLAYER_STATE::RUN:
        pAnimator->Play ( L"RUN" , true );
        break;
    case PLAYER_STATE::CROUCH:
        pAnimator->Play ( L"CROUCH" , true );
        break;
    case PLAYER_STATE::SLIDE:
        pAnimator->Play ( L"SLIDE" , false );
        break;
    case PLAYER_STATE::SLIDE_KICK_RECOIL:
        pAnimator->Play ( L"FALL1" , true );  // FALL1과 동일한 애니메이션
        break;
    case PLAYER_STATE::JUMP:
        pAnimator->Play ( L"JUMP" , false );
        break;
    case PLAYER_STATE::FALL0:
        pAnimator->Play ( L"FALL0" , true );
        break;
    case PLAYER_STATE::FALL1:
        pAnimator->Play ( L"FALL1" , true );
        break;
    case PLAYER_STATE::FALL2:
        pAnimator->Play ( L"FALL2" , true );
        break;
    case PLAYER_STATE::BOUNCE:
        pAnimator->Play ( L"BOUNCE" , false );
        break;
    case PLAYER_STATE::HOVER_EXHALE:
        pAnimator->Play ( L"HOVER_EXHALE" , false );
        break;
    case PLAYER_STATE::HOVER:
        switch ( m_eHoverSubState )
        {
        case HOVER_SUBSTATE::ENTER:
            pAnimator->Play ( L"HOVER_ENTER" , false );
            break;
        case HOVER_SUBSTATE::FLY_UP:
            pAnimator->Play ( L"HOVER_FLY" , false );
            break;
        case HOVER_SUBSTATE::FLOAT:
            pAnimator->Play ( L"HOVER_FLOAT" , true );
            break;
        case HOVER_SUBSTATE::GROUNDED:
            pAnimator->Play ( L"HOVER_GROUND" , true );
            break;
        default:
            pAnimator->Play ( L"HOVER_FLOAT" , true );
            break;
        }
        break;
    case PLAYER_STATE::DAMAGE:
    case PLAYER_STATE::MOUTHFUL_DAMAGE:
        pAnimator->Play ( L"DAMAGE" , false );
        break;
    case PLAYER_STATE::INHALE:
        pAnimator->Play ( L"INHALE" , true );
        break;
    case PLAYER_STATE::INHALE_SUCCESS:
        pAnimator->Play ( L"INHALE_SUCCESS" , false );
        break;
    case PLAYER_STATE::EXHALE:
        pAnimator->Play ( L"EXHALE" , false );
        break;
    case PLAYER_STATE::SWALLOW:
        pAnimator->Play ( L"SWALLOW" , false );
        break;
    case PLAYER_STATE::MOUTHFUL_IDLE:
        pAnimator->Play ( L"MOUTHFUL_IDLE" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_WALK:
        pAnimator->Play ( L"MOUTHFUL_WALK" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_RUN:
        pAnimator->Play ( L"MOUTHFUL_RUN" , true );
        break;
    case PLAYER_STATE::MOUTHFUL_JUMP:
        pAnimator->Play ( L"MOUTHFUL_JUMP" , false );
        break;
    case PLAYER_STATE::MOUTHFUL_FALL:
        pAnimator->Play ( L"MOUTHFUL_FALL" , true );
        break;
    default:
        char buffer[ 256 ];
        sprintf_s ( buffer , "WARNING: No animation case for state: %d\n" , ( int ) _eState );
        OutputDebugStringA ( buffer );
        break;
    }
}

bool CPlayerStateMachine::IsValidStateTransition ( PLAYER_STATE _from , PLAYER_STATE _to ) const
{
    // 같은 상태로의 전환은 허용하지 않음
    if ( _from == _to )
        return false;

    // 특정 상태에서는 특정 상태로만 전환 가능
    switch ( _from )
    {
    case PLAYER_STATE::SWALLOW:
        return false;

    case PLAYER_STATE::EXHALE:
        return false;

    case PLAYER_STATE::SLIDE:
        return ( _to == PLAYER_STATE::FALL0 ||
            _to == PLAYER_STATE::FALL1 ||
            _to == PLAYER_STATE::CROUCH ||
            _to == PLAYER_STATE::IDLE ||
            _to == PLAYER_STATE::SLIDE_KICK_RECOIL );

    case PLAYER_STATE::HOVER:
        return ( _to == PLAYER_STATE::HOVER_EXHALE );

    case PLAYER_STATE::HOVER_EXHALE:
        // 모든 전환 허용 - 전환 테이블에서 조건 관리
        return true;
    }

    // 기본적으로 모든 전환 허용
    return true;
}