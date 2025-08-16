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

public:
    // === 상태 관리 인터페이스 ===
    void ChangeStateInternal(PLAYER_STATE _eState);
    bool CanChangeToState(PLAYER_STATE _eState) const;

public:
    // === 상태 체크 함수들 ===
    bool IsInhaleState() const;
    bool IsMovingState() const;
    bool IsMouthfulState() const;
    bool IsGroundedState() const;

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

    // === 상태 변경 시 실행 ===
    void OnStateEnter(PLAYER_STATE _eState);
    void OnEnterJumpState();
    void OnEnterSlideState();
    void OnEnterInhaleState();
    void OnEnterBounceState();
    void OnEnterFallState();

private:
    // === 슬라이드 상태 처리 ===
    void InitiateSlide();           // 슬라이드 시작 처리
    void UpdateSlideMovement();     // 슬라이드 이동 처리
    void CheckSlideCompletion();    // 슬라이드 완료 체크
    void HandleSlideToFall();       // 슬라이드 중 낙하 처리

private:
    // === 공중 상태 처리 ===
    void HandleLanding();
    void PerformBounce();

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

    // === 멤버 변수들 ===
    CPlayer* m_pOwner;                              // 플레이어 참조
    CPlayerInputManager* m_pInputManager;           // 입력 매니저 (새로 추가)
    CPlayerStateTransitionTable* m_pTransitionTable; // 전환 테이블 (새로 추가)

    PLAYER_STATE    m_eCurState;    // 현재 상태
    PLAYER_STATE    m_ePrevState;   // 이전 상태
};