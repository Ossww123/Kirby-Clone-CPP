#pragma once

// 전방 선언
class CPlayer;
class CPlayerInputManager;
class CPlayerStateTransitionTable;
class CProjectile;

class CPlayerStateMachine
{
public:
    // === 생성자 & 소멸자 ===
    CPlayerStateMachine ( CPlayer* _pOwner );
    ~CPlayerStateMachine ( );

public:
    // === 핵심 생명주기 함수들 ===
    void Init ( );
    void Update ( );

private:
    // === 상태 변경 인터페이스 ===
    void ChangeStateInternal ( PLAYER_STATE _eState );

public:
    // 강제 필요한 경우에만 사용 (예: 게임오버, 씬 변환 등)
    void ForceStateChange ( PLAYER_STATE _eState );
    void ForceStateForSystemReset ( PLAYER_STATE _eState );

    bool CanChangeToState ( PLAYER_STATE _eState ) const;

public:
    // === 상태 체크 함수들 ===
    bool IsInhaleState ( ) const;
    bool IsMovingState ( ) const;
    bool IsMouthfulState ( ) const;
    bool IsGroundedState ( ) const;
    bool IsHoverState ( ) const;          // 현재 HOVER 상태인지
    bool IsHoverGrounded ( ) const;       // HOVER 중 땅에 있는지
    bool IsHoverFloating ( ) const;       // HOVER 중 공중에 있는지
    HOVER_SUBSTATE GetHoverSubState ( ) const { return m_eHoverSubState; }

public:
    // === DAMAGE 상태 체크 함수 ===
    bool IsDamageCompleted ( ) const { return m_bDamageCompleted; }

    // === 능력 획득 연출 체크 함수 ===
    bool IsAbilityAcquisitionAttack ( ) const { return m_bAbilityAcquisitionAttack; }

public:
    // === Getter 함수들 ===
    PLAYER_STATE GetCurrentState ( ) const { return m_eCurState; }
    PLAYER_STATE GetPreviousState ( ) const { return m_ePrevState; }
    CPlayerInputManager* GetInputManager ( ) const { return m_pInputManager; }
    INHALE_COUNT GetInhaleCount ( ) const { return m_eInhaleCount; }
    COPY_ABILITY GetCopyAbility ( ) const { return m_eCopyAbility; }
    COPY_ABILITY GetPendingCopyAbility ( ) const { return m_ePendingCopyAbility; }

    // === Setter 함수들 ===
    void SetInhaleCount ( INHALE_COUNT _eCount ) { m_eInhaleCount = _eCount; }
    void SetCopyAbility ( COPY_ABILITY _eAbility );
    void SetPendingCopyAbility ( COPY_ABILITY _eAbility ) { m_ePendingCopyAbility = _eAbility; }

    // === 연출용 공격 실행 ===
    void ExecutePresentationAttack ( );

private:
    // === 상태 실행 (입력 처리 포함, 상태 업데이트) ===
    void ExecuteCurrentState ( );
    void ExecuteIdleState ( );
    void ExecuteMovementState ( );
    void ExecuteJumpState ( );
    void ExecuteFallState ( );
    void ExecuteCrouchState ( );
    void ExecuteSlideState ( );
    void ExecuteInhaleState ( );
    void ExecuteInhaleKeepState ( );
    void ExecuteInhaleSuccessState ( );
    void ExecuteExhaleState ( );
    void ExecuteSwallowState ( );
    void ExecuteMouthfulStates ( );
    void ExecuteSpecialStates ( );
    void ExecuteBounceState ( );
    void ExecuteHoverState ( );
    void ExecuteHoverExhaleState ( );
    void ExecuteDamageState ( );
    void ExecuteAttackState ( );
    void ExecuteAttackHoldState ( );
    void ExecuteDoorEnterState ( );
    void ExecuteVictoryDanceState ( );

    // === 상태 진입/종료 시 처리 ===
    void OnStateEnter ( PLAYER_STATE _eState );
    void OnStateExit ( PLAYER_STATE _eState );
    void OnEnterIdleState ( );
    void OnEnterJumpState ( );
    void OnEnterWalkState ( );
    void OnEnterRunState ( );
    void OnEnterSlideState ( );
    void OnEnterInhaleState ( );
    void OnEnterInhaleKeepState ( );
    void OnEnterInhaleSuccessState ( );
    void OnEnterExhaleState ( );
    void OnEnterSwallowState ( );
    void OnEnterMouthfulState ( );
    void OnEnterBounceState ( );
    void OnEnterFallState ( );
    void OnEnterHoverState ( );           // HOVER 상태 시 처리
    void OnEnterHoverExhaleState ( );     // HOVER_EXHALE 상태 시 처리
    void OnExitHoverState ( );            // HOVER 상태 후 처리
    void OnEnterDamageState ( );
    void OnEnterVictoryDanceState ( );
    void OnEnterAttackState ( );

private:
    // === 카피 능력별 공격 함수들 ===
    void ExecuteFireAttack ( );      // 파이어 능력 공격 (0.5초)
    void ExecuteBeamAttack ( );      // 빔 능력 공격
    void ExecuteSparkAttack ( );     // 스파크 능력 공격 (0.5초)
    void ExecuteFireHoldAttack ( );  // 파이어 홀드 공격
    void ExecuteSparkHoldAttack ( ); // 스파크 홀드 공격

    // === 능력별 시스템 ===
    void HandleDropAbility ( );      // 백스페이스 키로 능력 버리기

private:
    // === 슬라이드 상태 처리 ===
    void InitiateSlide ( );           // 슬라이드 시작 처리
    void UpdateSlideMovement ( );     // 슬라이드 이동 처리
    void UpdateSlideKickProjectile ( ); // 슬라이드 킥 투사체 처리
    void CheckSlideCompletion ( );    // 슬라이드 완료 체크
    void HandleSlideToFall ( );       // 슬라이드 후 낙하 처리
    bool IsSlideCompleted ( ) const;

    // === 슬라이딩킥 반동 상태 처리 ===
    void ExecuteSlideKickRecoilState ( ); // 슬라이딩킥 반동 상태 처리
    void OnEnterSlideKickRecoilState ( ); // 슬라이딩킥 반동 상태 진입

private:
    // === 착지 상태 처리 ===
    void HandleLanding ( );
    void PerformBounce ( );

    // === HOVER 서브스테이트 업데이트 함수들 ===
    void UpdateHoverEnter ( );            // ENTER 서브스테이트 처리
    void UpdateHoverFlyUp ( );            // FLY_UP 서브스테이트 처리
    void UpdateHoverFloat ( );            // FLOAT 서브스테이트 처리
    void UpdateHoverGrounded ( );         // GROUNDED 서브스테이트 처리

    // === HOVER 이동 처리 함수들 ===
    void UpdateHoverMovement ( );         // HOVER 중 좌우 이동 처리
    void UpdateHoverPhysics ( );          // HOVER 물리 처리

    // === HOVER 유틸리티 함수들 ===
    void ApplyHoverUpForce ( );           // 상승력 적용
    void ApplyHoverFallSpeed ( );         // 천천히 낙하 적용
    void DisableGravityForHover ( );      // HOVER용 중력 비활성화
    void RestoreGravityFromHover ( );     // HOVER 종료 후 중력 복원

    // === HOVER 서브스테이트 관리 ===
    void ChangeHoverSubState ( HOVER_SUBSTATE _eNewSubState );
    void ResetHoverSubStateTimer ( ) { m_fHoverSubStateTimer = 0.0f; }

private:
    // === 애니메이션 관리 ===
    void SetAnimationForState ( PLAYER_STATE _eState );

private:
    // === 상태 변환 유효성 검사 ===
    bool IsValidStateTransition ( PLAYER_STATE _from , PLAYER_STATE _to ) const;

private:
    // === 변환 테이블 초기화 ===
    void InitializeTransitionTable ( );
    void AddBasicMovementTransitions ( );
    void AddJumpAndFallTransitions ( );
    void AddCrouchAndSlideTransitions ( );
    void AddInhaleTransitions ( );
    void AddSpecialTransitions ( );
    void AddHoverTransitions ( );
    void AddDamageTransitions ( );

private:
    // === 점프/낙하 상태 변수들 ===
    float m_fFallTime;              // 낙하 상태 경과 시간
    float m_fFall0Duration;         // FALL0 지속 시간 (0.3초)
    float m_fFallStartY;            // 낙하 시작 시 Y위치
    float m_fFallDistanceThreshold; // FALL2로 전환되는 낙하 거리 (224px = 3.5타일)
    float m_fBounceHeight;          // 바운스 시 점프 높이 (일반 점프의 70%)
    bool m_bWasGrounded;            // 이전 프레임에 땅에 있었는지

    // === 슬라이드 상태 변수들 ===
    float m_fSlideTimer;            // 슬라이드 경과 시간 타이머
    float m_fSlideDuration;         // 슬라이드 총 지속 시간
    float m_fSlideDistance;         // 슬라이드 이동할 총 거리
    float m_fSlideSpeed;            // 슬라이드 속도
    Vec2 m_vSlideStartPos;          // 슬라이드 시작 위치
    int m_iSlideDirection;          // 슬라이드 방향 (1: 오른쪽, -1: 왼쪽)
    bool m_bSlideGroundCheck;       // 슬라이드 중 땅에 체크 여부
    bool m_bSlideKickCreated;       // 슬라이드 킥 투사체 생성 여부

    // === 슬라이드킥 반동 상태 변수들 ===
    float m_fRecoilTimer;           // 반동 경과 타이머
    float m_fRecoilDuration;        // 반동 지속 시간 (0.3초)
    Vec2 m_vRecoilVelocity;         // 반동 속도 벡터

    // === HOVER 상태 관련 모든 변수들 ===
    HOVER_SUBSTATE  m_eHoverSubState;       // 현재 HOVER 서브스테이트
    float           m_fHoverUpForce;        // HOVER 상승력
    float           m_fHoverFallSpeed;      // HOVER 낙하 속도
    float           m_fHoverMoveSpeed;      // HOVER 이동 속도
    float           m_fHoverSubStateTimer;  // 서브스테이트 타이머
    float           m_fHoverEnterDuration;  // ENTER 단계 지속 시간
    float           m_fHoverFlyUpDuration;  // FLY_UP 단계 지속 시간
    float           m_fHoverAirFriction;    // 공중 상태 마찰
    bool            m_bHoverCanMoveOnGround; // 땅에서의 이동 가능 여부

    // === DAMAGE 상태 관련 변수들 ===
    float m_fDamageTimer;           // 피해 경과 타이머
    float m_fDamageDuration;        // 피해 상태 지속 시간 (0.5초)
    bool m_bDamageCompleted;        // 피해 시간 완료 플래그

    // === 능력 획득 연출 관련 ===
    bool m_bAbilityAcquisitionAttack;  // 능력 획득 연출 중인 공격인지

    // === 흡입들이기 관련 상태 변수들 ===
    INHALE_COUNT m_eInhaleCount;        // 흡입들이개 개수
    COPY_ABILITY m_eCopyAbility;        // 카피 능력 타입
    COPY_ABILITY m_ePendingCopyAbility; // 대기 중인 획득 카피 능력
    float m_fInhaleTimer;               // 흡입들이기 경과 타이머
    float m_fInhaleDuration;            // 흡입들이기 지속 시간
    float m_fInhaleSuccessTimer;        // 흡입들이기 성공 경과 타이머
    float m_fInhaleSuccessDuration;     // 흡입들이기 성공 지속 시간
    float m_fExhaleTimer;               // 토해내기 경과 타이머
    float m_fExhaleDuration;            // 토해내기 지속 시간
    float m_fSwallowTimer;              // 삼키기 경과 타이머
    float m_fSwallowDuration;           // 삼키기 지속 시간

    // === 공격 상태 관련 모든 변수 ===
    float m_fAttackTimer;               // 공격 경과 타이머
    float m_fAttackDuration;            // 공격 지속 시간

    // === 스파크 능력 전기장 관리 ===
    CProjectile* m_pElectricField;      // 현재 활성화된 전기장 투사체

    // === 핵심 객체들 ===
    CPlayer* m_pOwner;                              // 플레이어 소유자
    CPlayerInputManager* m_pInputManager;           // 입력 매니저 (필수 추가)
    CPlayerStateTransitionTable* m_pTransitionTable; // 전환 테이블 (필수 추가)

    PLAYER_STATE    m_eCurState;    // 현재 상태
    PLAYER_STATE    m_ePrevState;   // 이전 상태

    friend class CPlayer;
};