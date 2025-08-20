#pragma once

// ���� ����
class CPlayer;
class CPlayerInputManager;
class CPlayerStateTransitionTable;

class CPlayerStateMachine
{
public:
    // === ������ & �Ҹ��� ===
    CPlayerStateMachine(CPlayer* _pOwner);
    ~CPlayerStateMachine();

public:
    // === �ٽ� �����ֱ� �Լ��� ===
    void Init();
    void Update();

private:
    // === ���� ���� �������̽� ===
    void ChangeStateInternal(PLAYER_STATE _eState);

public:
    // ���� �ʿ��� ��쿡�� ��� (��: ���ӿ���, �� ��ȯ ��)
    void ForceStateChange(PLAYER_STATE _eState);
    void ForceStateForSystemReset(PLAYER_STATE _eState);

    bool CanChangeToState(PLAYER_STATE _eState) const;

public:
    // === ���� üũ �Լ��� ===
    bool IsInhaleState() const;
    bool IsMovingState() const;
    bool IsMouthfulState() const;
    bool IsGroundedState() const;
    bool IsHoverState() const;          // ���� HOVER ��������
    bool IsHoverGrounded() const;       // HOVER �� ���� �ִ���
    bool IsHoverFloating() const;       // HOVER �� ���߿� �ִ���
    HOVER_SUBSTATE GetHoverSubState() const { return m_eHoverSubState; }

public:
    // === DAMAGE ���� üũ �Լ� ===
    bool IsDamageCompleted() const { return m_bDamageCompleted; }

public:
    // === Getter �Լ��� ===
    PLAYER_STATE GetCurrentState() const { return m_eCurState; }
    PLAYER_STATE GetPreviousState() const { return m_ePrevState; }
    CPlayerInputManager* GetInputManager() const { return m_pInputManager; }
    INHALE_COUNT GetInhaleCount() const { return m_eInhaleCount; }
    COPY_ABILITY GetCopyAbility() const { return m_eCopyAbility; }
    
    // === Setter 함수들 ===
    void SetInhaleCount(INHALE_COUNT _eCount) { m_eInhaleCount = _eCount; }
    void SetCopyAbility(COPY_ABILITY _eAbility) { m_eCopyAbility = _eAbility; }

private:
    // === ���� ���� (�Է� ó�� ����, ���� ���ุ) ===
    void ExecuteCurrentState();
    void ExecuteIdleState();
    void ExecuteMovementState();
    void ExecuteJumpState();
    void ExecuteFallState();
    void ExecuteCrouchState();
    void ExecuteSlideState();
    void ExecuteInhaleState();
    void ExecuteInhaleSuccessState();
    void ExecuteExhaleState();
    void ExecuteSwallowState();
    void ExecuteMouthfulStates();
    void ExecuteSpecialStates();
    void ExecuteBounceState();
    void ExecuteHoverState();
    void ExecuteHoverExhaleState();
    void ExecuteDamageState();

    // === ���� ���� �� ���� ===
    void OnStateEnter(PLAYER_STATE _eState);
    void OnEnterJumpState();
    void OnEnterSlideState();
    void OnEnterInhaleState();
    void OnEnterInhaleSuccessState();
    void OnEnterExhaleState();
    void OnEnterSwallowState();
    void OnEnterMouthfulState();
    void OnEnterBounceState();
    void OnEnterFallState();
    void OnEnterHoverState();           // HOVER ���� �� ó��
    void OnEnterHoverExhaleState();     // HOVER_EXHALE ���� �� ó��
    void OnExitHoverState();            // HOVER ���� �� ó��
    void OnEnterDamageState();

private:
    // === �����̵� ���� ó�� ===
    void InitiateSlide();           // �����̵� ���� ó��
    void UpdateSlideMovement();     // �����̵� �̵� ó��
    void UpdateSlideKickProjectile(); // �����̵� ų ����ü ó��
    void CheckSlideCompletion();    // �����̵� �Ϸ� üũ
    void HandleSlideToFall();       // �����̵� �� ���� ó��
    bool IsSlideCompleted() const;
    
    // === �����̵�ų �ݵ� ���� ó�� ===
    void ExecuteSlideKickRecoilState(); // �����̵�ų �ݵ� ���� ó��
    void OnEnterSlideKickRecoilState(); // �����̵�ų �ݵ� ���� ����

private:
    // === ���� ���� ó�� ===
    void HandleLanding();
    void PerformBounce();

    // === HOVER ���꽺����Ʈ ������Ʈ �Լ��� ===
    void UpdateHoverEnter();            // ENTER ���꽺����Ʈ ó��
    void UpdateHoverFlyUp();            // FLY_UP ���꽺����Ʈ ó��
    void UpdateHoverFloat();            // FLOAT ���꽺����Ʈ ó��
    void UpdateHoverGrounded();         // GROUNDED ���꽺����Ʈ ó��

    // === HOVER ���� ó�� �Լ��� ===
    void UpdateHoverMovement();         // HOVER �� �¿� �̵� ó��
    void UpdateHoverPhysics();          // HOVER ���� ó��

    // === HOVER ��ƿ��Ƽ �Լ��� ===
    void ApplyHoverUpForce();           // ��·� ����
    void ApplyHoverFallSpeed();         // ���� ���� ����
    void DisableGravityForHover();      // HOVER�� �߷� ��Ȱ��ȭ
    void RestoreGravityFromHover();     // HOVER ���� �� �߷� ����

    // === HOVER ���꽺����Ʈ ���� ===
    void ChangeHoverSubState(HOVER_SUBSTATE _eNewSubState);
    void ResetHoverSubStateTimer() { m_fHoverSubStateTimer = 0.0f; }

private:
    // === �ִϸ��̼� ���� ===
    void SetAnimationForState(PLAYER_STATE _eState);

private:
    // === ���� ��ȯ ��ȿ�� �˻� ===
    bool IsValidStateTransition(PLAYER_STATE _from, PLAYER_STATE _to) const;

private:
    // === ��ȯ ���̺� �ʱ�ȭ ===
    void InitializeTransitionTable();
    void AddBasicMovementTransitions();
    void AddJumpAndFallTransitions();
    void AddCrouchAndSlideTransitions();
    void AddInhaleTransitions();
    void AddSpecialTransitions();
    void AddHoverTransitions();
    void AddDamageTransitions();

private:
    // === ����/���� ���� ������ ===
    float m_fFallTime;              // ���� ���� ���� �ð�
    float m_fFall0Duration;         // FALL0 ���� �ð� (0.3��)
    float m_fFallStartY;            // ���� ���� �� Y��ġ
    float m_fFallDistanceThreshold; // FALL2�� ��ȯ�Ǵ� ���� �Ÿ� (224px = 3.5Ÿ��)
    float m_fBounceHeight;          // �ٿ �� ���� ���� (�Ϲ� ������ 70%)
    bool m_bWasGrounded;            // ���� �����ӿ� ���� �־�����

    // === �����̵� ���� ������ ===
    float m_fSlideTimer;            // �����̵� ���� �ð� Ÿ�̸�
    float m_fSlideDuration;         // �����̵� �� ���� �ð�
    float m_fSlideDistance;         // �����̵� �̵��� �� �Ÿ�
    float m_fSlideSpeed;            // �����̵� �ӵ�
    Vec2 m_vSlideStartPos;          // �����̵� ���� ��ġ
    int m_iSlideDirection;          // �����̵� ���� (1: ������, -1: ����)
    bool m_bSlideGroundCheck;       // �����̵� �� ���� üũ ����
    bool m_bSlideKickCreated;       // �����̵� ų ����ü ���� ����
    
    // === �����̵�ų �ݵ� ���� ������ ===
    float m_fRecoilTimer;           // �ݵ� ���� Ÿ�̸�
    float m_fRecoilDuration;        // �ݵ� ���� �ð� (0.3��)
    Vec2 m_vRecoilVelocity;         // �ݵ� �ӵ� ����

    // === HOVER ���� ���� ��� ������ ===
    HOVER_SUBSTATE  m_eHoverSubState;       // ���� HOVER ���꽺����Ʈ
    float           m_fHoverUpForce;        // HOVER ��·�
    float           m_fHoverFallSpeed;      // HOVER ���� �ӵ�
    float           m_fHoverMoveSpeed;      // HOVER �̵� �ӵ�
    float           m_fHoverSubStateTimer;  // ���꽺����Ʈ Ÿ�̸�
    float           m_fHoverEnterDuration;  // ENTER ���� ���� �ð�
    float           m_fHoverFlyUpDuration;  // FLY_UP ���� ���� �ð�
    float           m_fHoverAirFriction;    // ���� ���� ���
    bool            m_bHoverCanMoveOnGround; // �������� �̵� ���� ����

    // === DAMAGE ���� ���� ������ ===
    float m_fDamageTimer;           // �ǰ� ���� Ÿ�̸�
    float m_fDamageDuration;        // �ǰ� ���� ���� �ð� (0.5��)
    bool m_bDamageCompleted;        // �ǰ� �ð� �Ϸ� �÷���

    // === ���ȴ��̱� ���� ���� ������ ===
    INHALE_COUNT m_eInhaleCount;        // ���ȴ��̰� ����
    COPY_ABILITY m_eCopyAbility;        // ī�� ���� Ÿ��
    float m_fInhaleTimer;               // ���ȴ��̱� ���� Ÿ�̸�
    float m_fInhaleDuration;            // ���ȴ��̱� ���� �ð�
    float m_fInhaleSuccessTimer;        // ���ȴ��̱� ���� ���� Ÿ�̸�
    float m_fInhaleSuccessDuration;     // ���ȴ��̱� ���� ���� �ð�
    float m_fExhaleTimer;               // �Ͱ� ���� Ÿ�̸�
    float m_fExhaleDuration;            // �Ͱ� ���� �ð�
    float m_fSwallowTimer;              // ��Ű�� ���� Ÿ�̸�
    float m_fSwallowDuration;           // ��Ű�� ���� �ð�

    // === ��� ������ ===
    CPlayer* m_pOwner;                              // �÷��̾� ����
    CPlayerInputManager* m_pInputManager;           // �Է� �Ŵ��� (���� �߰�)
    CPlayerStateTransitionTable* m_pTransitionTable; // ��ȯ ���̺� (���� �߰�)

    PLAYER_STATE    m_eCurState;    // ���� ����
    PLAYER_STATE    m_ePrevState;   // ���� ����



    friend class CPlayer;
};