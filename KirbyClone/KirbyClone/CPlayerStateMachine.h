#pragma once

// 전방 선언
class CPlayer;
class CPlayerInputManager;
class CPlayerStateTransitionTable;

class CPlayerStateMachine
{
public:
    // === 생성자 & 소멸자 ===
    CPlayerStateMachine(CPlayer* _pOwner);
    ~CPlayerStateMachine();

public:
    // === 핵심 생명주기 함수들 ===
    void Init();
    void Update();

private:
    // === 상태 관리 인터페이스 ===
    void ChangeStateInternal(PLAYER_STATE _eState);

public:
    // 정말 필요한 경우에만 사용 (예: 게임오버, 씬 전환 등)
    void ForceStateChange(PLAYER_STATE _eState);
    void ForceStateForSystemReset(PLAYER_STATE _eState);

    bool CanChangeToState(PLAYER_STATE _eState) const;

public:
    // === 상태 체크 함수들 ===
    bool IsInhaleState() const;
    bool IsMovingState() const;
    bool IsMouthfulState() const;
    bool IsGroundedState() const;
    bool IsHoverState() const;          // 현재 HOVER 상태인지
    bool IsHoverGrounded() const;       // HOVER 중 땅에 있는지
    bool IsHoverFloating() const;       // HOVER 중 공중에 있는지
    HOVER_SUBSTATE GetHoverSubState() const { return m_eHoverSubState; }

public:
    // === DAMAGE 상태 체크 함수 ===
    bool IsDamageCompleted() const { return m_bDamageCompleted; }

public:
    // === Getter 함수들 ===
    PLAYER_STATE GetCurrentState() const { return m_eCurState; }
    PLAYER_STATE GetPreviousState() const { return m_ePrevState; }
    CPlayerInputManager* GetInputManager() const { return m_pInputManager; }

private:
    // === 상태 실행 (입력 처리 없음, 순수 실행만) ===
    void ExecuteCurrentState();
    void ExecuteIdleState();
    void ExecuteMovementState();
    void ExecuteJumpState();
    void ExecuteFallState();
    void ExecuteCrouchState();
    void ExecuteSlideState();
    void ExecuteInhaleStates();
    void ExecuteSpecialStates();
    void ExecuteBounceState();
    void ExecuteHoverState();
    void ExecuteHoverExhaleState();
    void ExecuteDamageState();

    // === 상태 변경 시 실행 ===
    void OnStateEnter(PLAYER_STATE _eState);
    void OnEnterJumpState();
    void OnEnterSlideState();
    void OnEnterInhaleState();
    void OnEnterBounceState();
    void OnEnterFallState();
    void OnEnterHoverState();           // HOVER 진입 시 처리
    void OnEnterHoverExhaleState();     // HOVER_EXHALE 진입 시 처리
    void OnExitHoverState();            // HOVER 종료 시 처리
    void OnEnterDamageState();

private:
    // === 슬라이드 상태 처리 ===
    void InitiateSlide();           // 슬라이드 시작 처리
    void UpdateSlideMovement();     // 슬라이드 이동 처리
    void CheckSlideCompletion();    // 슬라이드 완료 체크
    void HandleSlideToFall();       // 슬라이드 중 낙하 처리
    bool IsSlideCompleted() const;

private:
    // === 공중 상태 처리 ===
    void HandleLanding();
    void PerformBounce();

    // === HOVER 서브스테이트 업데이트 함수들 ===
    void UpdateHoverEnter();            // ENTER 서브스테이트 처리
    void UpdateHoverFlyUp();            // FLY_UP 서브스테이트 처리
    void UpdateHoverFloat();            // FLOAT 서브스테이트 처리
    void UpdateHoverGrounded();         // GROUNDED 서브스테이트 처리

    // === HOVER 공통 처리 함수들 ===
    void UpdateHoverMovement();         // HOVER 중 좌우 이동 처리
    void UpdateHoverPhysics();          // HOVER 물리 처리

    // === HOVER 유틸리티 함수들 ===
    void ApplyHoverUpForce();           // 상승력 적용
    void ApplyHoverFallSpeed();         // 느린 낙하 적용
    void DisableGravityForHover();      // HOVER용 중력 비활성화
    void RestoreGravityFromHover();     // HOVER 종료 시 중력 복구

    // === HOVER 서브스테이트 관리 ===
    void ChangeHoverSubState(HOVER_SUBSTATE _eNewSubState);
    void ResetHoverSubStateTimer() { m_fHoverSubStateTimer = 0.0f; }

private:
    // === 애니메이션 설정 ===
    void SetAnimationForState(PLAYER_STATE _eState);

private:
    // === 상태 전환 유효성 검사 ===
    bool IsValidStateTransition(PLAYER_STATE _from, PLAYER_STATE _to) const;

private:
    // === 전환 테이블 초기화 ===
    void InitializeTransitionTable();
    void AddBasicMovementTransitions();
    void AddJumpAndFallTransitions();
    void AddCrouchAndSlideTransitions();
    void AddInhaleTransitions();
    void AddSpecialTransitions();
    void AddHoverTransitions();
    void AddDamageTransitions();

private:
    // === 점프/낙하 관련 변수들 ===
    float m_fFallTime;              // 현재 낙하 지속 시간
    float m_fFallToBounceThreshold; // FALL2로 전환되는 시간 (1.0초)
    float m_fBounceHeight;          // 바운스 시 점프 높이 (일반 점프의 70%)
    bool m_bWasGrounded;            // 이전 프레임에 땅에 있었는지

    // === 슬라이드 관련 변수들 ===
    float m_fSlideTimer;            // 슬라이드 지속 시간 타이머
    float m_fSlideDuration;         // 슬라이드 총 지속 시간
    float m_fSlideDistance;         // 슬라이드 이동할 총 거리
    float m_fSlideSpeed;            // 슬라이드 속도
    Vec2 m_vSlideStartPos;          // 슬라이드 시작 위치
    int m_iSlideDirection;          // 슬라이드 방향 (1: 오른쪽, -1: 왼쪽)
    bool m_bSlideGroundCheck;       // 슬라이드 중 지면 체크 여부

    // === HOVER 상태 관리 멤버 변수들 ===
    HOVER_SUBSTATE  m_eHoverSubState;       // 현재 HOVER 서브스테이트
    float           m_fHoverUpForce;        // HOVER 상승력
    float           m_fHoverFallSpeed;      // HOVER 낙하 속도
    float           m_fHoverMoveSpeed;      // HOVER 이동 속도
    float           m_fHoverSubStateTimer;  // 서브스테이트 타이머
    float           m_fHoverEnterDuration;  // ENTER 상태 지속 시간
    float           m_fHoverFlyUpDuration;  // FLY_UP 상태 지속 시간
    float           m_fHoverAirFriction;    // 공중 마찰 계수
    bool            m_bHoverCanMoveOnGround; // 땅에서도 이동 가능 여부

    // === DAMAGE 상태 관련 변수들 ===
    float m_fDamageTimer;           // 피격 상태 타이머
    float m_fDamageDuration;        // 피격 상태 지속 시간 (0.5초)
    bool m_bDamageCompleted;        // 피격 시간 완료 플래그

    // === 멤버 변수들 ===
    CPlayer* m_pOwner;                              // 플레이어 참조
    CPlayerInputManager* m_pInputManager;           // 입력 매니저 (새로 추가)
    CPlayerStateTransitionTable* m_pTransitionTable; // 전환 테이블 (새로 추가)

    PLAYER_STATE    m_eCurState;    // 현재 상태
    PLAYER_STATE    m_ePrevState;   // 이전 상태



    friend class CPlayer;
};