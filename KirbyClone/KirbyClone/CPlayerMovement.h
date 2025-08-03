#pragma once
#include "pch.h"

class CPlayer;
class CRigidBody;

class CPlayerMovement
{
private:
    CPlayer* m_pOwner;          // 플레이어 참조
    CRigidBody* m_pRigidBody;   // 리지드바디 참조

    // === 이동 관련 변수들 ===
    float m_fSpeed;             // 기본 이동 속도
    float m_fRunSpeed;          // 달리기 속도
    float m_fJumpPower;         // 점프력
    bool m_bRunMode;            // 달리기 모드인지

    // === 방향 시스템 관련 변수들 ===
    bool m_bFacingRight;        // 오른쪽을 보고 있는지 (true: 오른쪽, false: 왼쪽)
    int m_iLastMoveDir;         // 마지막 이동 방향 (1: 오른쪽, -1: 왼쪽, 0: 정지)
    bool m_bDirectionChanged;   // 방향이 바뀌었는지 체크

    // === 부드러운 움직임 관련 변수들 ===
    float m_fDeceleration;      // 감속도 (키를 뗄 때)
    float m_fMinMovingSpeed;    // 최소 이동 속도 (이 이하면 정지로 간주)
    bool m_bIsDecelerating;     // 현재 감속 중인지

    // === 입력 상태 추적 ===
    bool m_bWasMovingLastFrame; // 이전 프레임에 이동 중이었는지
    bool m_bInputPressed;       // 현재 입력이 눌려있는지

public:
    CPlayerMovement(CPlayer* _pOwner);
    ~CPlayerMovement();

public:
    // === 초기화 ===
    void Init(CRigidBody* _pRigidBody);

    // === 업데이트 함수들 ===
    void Update();
    void HandleMovementInput();
    void HandleJumpInput();
    void UpdateDirection();
    void UpdateMovementState();
    void ApplyDeceleration();

    // === 방향 관련 ===
    void SetFacingDirection(bool _bRight);
    bool IsFacingRight() const { return m_bFacingRight; }
    bool IsDirectionChanged() const { return m_bDirectionChanged; }

    // === 이동 상태 체크 ===
    bool IsActuallyMoving() const;
    bool IsInputPressed() const { return m_bInputPressed; }
    bool IsRunMode() const { return m_bRunMode; }
    void SetRunMode(bool _bRunMode) { m_bRunMode = _bRunMode; }

    // === 속도 관련 ===
    float GetCurrentSpeed() const;
    float GetSpeed() const { return m_fSpeed; }
    float GetRunSpeed() const { return m_fRunSpeed; }
    void SetSpeed(float _fSpeed) { m_fSpeed = _fSpeed; }
    void SetRunSpeed(float _fRunSpeed) { m_fRunSpeed = _fRunSpeed; }

    // === 점프 관련 ===
    float GetJumpPower() const { return m_fJumpPower; }
    void SetJumpPower(float _fJumpPower) { m_fJumpPower = _fJumpPower; }
    void Jump();
    bool CanJump() const;

    // === 감속 관련 ===
    void SetDeceleration(float _fDeceleration) { m_fDeceleration = _fDeceleration; }
    void SetMinMovingSpeed(float _fMinSpeed) { m_fMinMovingSpeed = _fMinSpeed; }
    bool IsDecelerating() const { return m_bIsDecelerating; }

    // === 속도 직접 제어 (특수 상황용) ===
    void StopMovement();
    void SetVelocityX(float _fVelX);
};