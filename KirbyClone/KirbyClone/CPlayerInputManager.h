#pragma once

class CPlayerInputManager
{
public:
    enum class INPUT_TYPE : uint32_t
    {
        // === 기본 입력들 ===
        MOVE_LEFT = 1 << 0,        // LEFT 키
        MOVE_RIGHT = 1 << 1,       // RIGHT 키  
        MOVE_UP = 1 << 2,          // UP 키
        MOVE_DOWN = 1 << 3,        // DOWN 키 (크라우치)

        JUMP = 1 << 4,             // Z 키 (점프)
        ACTION = 1 << 5,           // X 키 (카피능력/슬라이드/흡입)
        DROP_ABILITY = 1 << 6,     // BACKSPACE 키 (능력 버리기)

        // === TAP, HOLD, AWAY 구분 ===
        JUMP_TAP = 1 << 8,         // Z 키 TAP (방금 눌렀음)
        JUMP_HOLD = 1 << 9,        // Z 키 HOLD (계속 누르고 있음)
        JUMP_AWAY = 1 << 10,       // Z 키 AWAY (방금 뗐음)

        ACTION_TAP = 1 << 11,      // X 키 TAP
        ACTION_HOLD = 1 << 12,     // X 키 HOLD  
        ACTION_AWAY = 1 << 13,     // X 키 AWAY

        DROP_ABILITY_TAP = 1 << 14, // BACKSPACE 키 TAP

        // === 방향키 AWAY (필요시) ===
        MOVE_LEFT_AWAY = 1 << 16,   // LEFT 키 AWAY
        MOVE_RIGHT_AWAY = 1 << 17,  // RIGHT 키 AWAY

        // === 더블탭 감지 ===
        DOUBLE_TAP_LEFT = 1 << 16,
        DOUBLE_TAP_RIGHT = 1 << 17,

        // === 조합 입력 (자주 사용되는 것들) ===
        MOVE_HORIZONTAL = MOVE_LEFT | MOVE_RIGHT,
        ANY_MOVEMENT = MOVE_LEFT | MOVE_RIGHT | MOVE_UP | MOVE_DOWN
    };

    using InputFlags = uint32_t;

public:
    CPlayerInputManager();
    ~CPlayerInputManager();

public:
    // === 핵심 인터페이스 ===
    void Update();                              // 매 프레임 호출
    InputFlags GetCurrentFrameInput() const;   // 현재 프레임 입력
    bool HasInput(InputFlags flags) const;     // 특정 입력이 있는지 체크
    bool HasAllInputs(InputFlags flags) const; // 모든 입력이 있는지 체크
    bool HasAnyInput(InputFlags flags) const;  // 어떤 입력이라도 있는지 체크

public:
    // === 개별 입력 체크 (편의 함수들) ===
    bool IsMovingLeft() const { return HasInput((uint32_t)INPUT_TYPE::MOVE_LEFT); }
    bool IsMovingRight() const { return HasInput((uint32_t)INPUT_TYPE::MOVE_RIGHT); }
    bool IsMovingUp() const { return HasInput((uint32_t)INPUT_TYPE::MOVE_UP); }
    bool IsMovingDown() const { return HasInput((uint32_t)INPUT_TYPE::MOVE_DOWN); }
    // === 점프 높이 조절 관련 ===
    bool IsJumpTap() const { return HasInput((uint32_t)INPUT_TYPE::JUMP_TAP); }
    bool IsJumpHold() const { return HasInput((uint32_t)INPUT_TYPE::JUMP_HOLD); }
    bool IsJumpAway() const { return HasInput((uint32_t)INPUT_TYPE::JUMP_AWAY); }
    float GetJumpHoldRatio() const;            // 점프 키를 누른 비율 (0.0~1.0)

    // === 액션 키 관련 ===
    bool IsActionTap() const { return HasInput((uint32_t)INPUT_TYPE::ACTION_TAP); }
    bool IsActionHold() const { return HasInput((uint32_t)INPUT_TYPE::ACTION_HOLD); }
    bool IsActionAway() const { return HasInput((uint32_t)INPUT_TYPE::ACTION_AWAY); }

    // === 기타 ===
    bool IsDropAbilityTap() const { return HasInput((uint32_t)INPUT_TYPE::DROP_ABILITY_TAP); }

    // === 더블탭 체크 ===
    bool IsDoubleTapLeft() const { return HasInput((uint32_t)INPUT_TYPE::DOUBLE_TAP_LEFT); }
    bool IsDoubleTapRight() const { return HasInput((uint32_t)INPUT_TYPE::DOUBLE_TAP_RIGHT); }

    // === 방향 입력 체크 ===
    int GetHorizontalInput() const;    // -1(왼쪽), 0(없음), 1(오른쪽)
    int GetVerticalInput() const;      // -1(아래), 0(없음), 1(위)

private:
    // === 입력 수집 함수들 ===
    void CollectKeyboardInput();
    void ProcessDoubleTap();
    void UpdateInputFlags();

    // === 더블탭 관련 내부 함수들 ===
    void CheckMovementStyleDoubleTap();

private:
    // === 현재 프레임 입력 상태 ===
    InputFlags m_currentFrameInput;     // 이번 프레임 입력

    // === 감지용 변수들 ===
    bool m_bWasJumpPressed;
    bool m_bWasActionPressed;

    // Movement 스타일 더블탭용 추가 변수들
    bool m_bLeftDecelerating;           // 왼쪽 키 감속 중
    bool m_bRightDecelerating;          // 오른쪽 키 감속 중
    int m_iLastMoveDirection;           // 마지막 이동 방향

    // === 점프 높이 조절용 변수들 ===
    float m_fJumpHoldTime;              // Z키를 누르고 있는 시간
    float m_fMaxJumpHoldTime;           // 최대 점프 홀드 시간 (0.3초 정도)
    bool m_bLeftCurrentlyPressed;       // 현재 LEFT 키 상태
    bool m_bRightCurrentlyPressed;      // 현재 RIGHT 키 상태
    bool m_bJumpCurrentlyPressed;       // 현재 Z 키 상태 (점프 높이 조절용)
    bool m_bActionCurrentlyPressed;
};