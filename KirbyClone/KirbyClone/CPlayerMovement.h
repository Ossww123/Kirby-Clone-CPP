#pragma once
#include "pch.h"

class CPlayer;
class CPlayerInputManager;

class CPlayerMovement
{
public:
    // === 생성자 & 소멸자 ===
    CPlayerMovement(CPlayer* _pOwner);
    ~CPlayerMovement();

public:
    // === 핵심 생명주기 함수들 ===
    void Init();
    void Update();

private:
    // === 입력 처리 함수들 ===
    void ApplyCurrentMovement();
    void ProcessMovementInput();
    void ProcessMovementInputLegacy();

private:
    // === 방향 관리 함수들 ===
    void UpdateDirection();
    void UpdateMovementState();

private:
    // === 감속 관리 함수들 ===
    void StartDeceleration(int _iDirection);
    void ApplyDeceleration();
    void ApplyCrouchDeceleration();

private:
    // === 더블탭 시스템 함수들 ===
    bool CheckDoubleTap(int _iCurrentDir);
    void ResetDoubleTapState();

public:
    // === 방향 관련 ===
    void SetFacingDirection(bool _bRight);

public:
    // === 이동 상태 체크 ===
    bool IsActuallyMoving() const;
    bool IsInputPressed() const { return m_bInputPressed; }
    bool IsRunMode() const { return m_bRunMode; }
    void SetRunMode(bool _bRunMode) { m_bRunMode = _bRunMode; }

public:
    // === 크라우치 관련 ===
    void HandleCrouchDirectionInput();

public:
    // === 점프 관련 ===
    void Jump();
    bool CanJump() const;

public:
    // === 슬라이드 관련 ===
    bool CanSlide() const;
    void InitiateSlide();

public:
    // === 속도 직접 제어 (특수 상황용) ===
    void StopMovement();
    void SetVelocityX(float _fVelX);

public:
    // === Getter 함수들 ===
    bool IsFacingRight() const { return m_bFacingRight; }
    bool IsDirectionChanged() const { return m_bDirectionChanged; }
    float GetCurrentSpeed() const;
    float GetSpeed() const { return m_fSpeed; }
    float GetRunSpeed() const { return m_fRunSpeed; }
    float GetJumpPower() const { return m_fJumpPower; }
    bool IsDecelerating() const { return m_bIsDecelerating; }

public:
    // === Setter 함수들 ===
    void SetSpeed(float _fSpeed) { m_fSpeed = _fSpeed; }
    void SetRunSpeed(float _fRunSpeed) { m_fRunSpeed = _fRunSpeed; }
    void SetJumpPower(float _fJumpPower) { m_fJumpPower = _fJumpPower; }
    void SetDeceleration(float _fDeceleration) { m_fDeceleration = _fDeceleration; }
    void SetMinMovingSpeed(float _fMinSpeed) { m_fMinMovingSpeed = _fMinSpeed; }

public:
    // === InputManager 연결 ===
    void SetInputManager(CPlayerInputManager* pInputManager) { m_pInputManager = pInputManager; }

private:
    // === 소유자 참조 ===
    CPlayer* m_pOwner;                          // 플레이어 참조
    CPlayerInputManager* m_pInputManager;       // 입력 매니저

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

    // === 크라우치 관련 변수들 ===
    float m_fCrouchDeceleration; // 크라우치 상태 감속도 (더 빠르게)

    // === 더블탭 RUN 시스템 관련 변수들 ===
    float m_fDoubleTapWindow;       // 더블탭 인식 시간 (0.3초)
    int m_iDeceleratingDirection;   // 감속 중인 방향 (-1: 왼쪽, 1: 오른쪽, 0: 없음)

    // === 입력 상태 추적 ===
    bool m_bWasMovingLastFrame; // 이전 프레임에 이동 중이었는지
    bool m_bInputPressed;       // 현재 입력이 눌려있는지
};