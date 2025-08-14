#pragma once

// 전방 선언
class CAnimator;
class CRigidBody;
class CPlayer;

class CPlayerStateMachine
{
public:
    // === 생성자 & 소멸자 ===
    CPlayerStateMachine(CPlayer* _pOwner);
    ~CPlayerStateMachine();

public:
    // === 핵심 생명주기 함수들 ===
    void Init(CAnimator* _pAnimator, CRigidBody* _pRigidBody);
    void Update();

public:
    // === 상태 관리 인터페이스 ===
    void ChangeState(PLAYER_STATE _eState);
    bool CanChangeToState(PLAYER_STATE _eState) const;

private:
    // === 상태 업데이트 로직들 ===
    void UpdateIdleState();
    void UpdateMovementState();
    void UpdateInhaleState();
    void UpdateAirborneState();
    void UpdateSpecialState();

public:
    // === 상태 체크 함수들 ===
    bool IsInhaleState() const;
    bool IsMovingState() const;
    bool IsMouthfulState() const;
    bool IsGroundedState() const;

private:
    // === 애니메이션 설정 ===
    void SetAnimationForState(PLAYER_STATE _eState);

public:
    // === Getter 함수들 ===
    PLAYER_STATE GetCurrentState() const { return m_eCurState; }
    PLAYER_STATE GetPreviousState() const { return m_ePrevState; }

private:
    // === 상태 전환 유효성 검사 ===
    bool IsValidStateTransition(PLAYER_STATE _from, PLAYER_STATE _to) const;

private:
    // === 멤버 변수들 ===
    CPlayer* m_pOwner;       // 플레이어 참조
    CAnimator* m_pAnimator;    // 애니메이터 참조
    CRigidBody* m_pRigidBody;   // 리지드바디 참조

    PLAYER_STATE    m_eCurState;    // 현재 상태
    PLAYER_STATE    m_ePrevState;   // 이전 상태
};