#include "pch.h"
#include "CPlayerInputManager.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"

CPlayerInputManager::CPlayerInputManager()
    : m_currentFrameInput(0)
    , m_bWasJumpPressed(false)
    , m_bWasActionPressed(false)
    , m_fJumpHoldTime(0.0f)
    , m_fMaxJumpHoldTime(0.3f)
    , m_bLeftCurrentlyPressed(false)
    , m_bRightCurrentlyPressed(false)
    , m_bJumpCurrentlyPressed(false)
    , m_bActionCurrentlyPressed(false)
    , m_bLeftDecelerating(false)
    , m_bRightDecelerating(false)
    , m_iLastMoveDirection(0)
{
}

CPlayerInputManager::~CPlayerInputManager()
{
}

void CPlayerInputManager::Update()
{
    // 이전 프레임 입력 저장
    m_bWasJumpPressed = m_bJumpCurrentlyPressed;
    m_bWasActionPressed = m_bActionCurrentlyPressed;

    // 현재 프레임 입력 초기화
    m_currentFrameInput = 0;

    // 키보드 입력 수집
    CollectKeyboardInput();

    // 더블탭 처리
    ProcessDoubleTap();

    // 입력 플래그 업데이트
    UpdateInputFlags();
}

CPlayerInputManager::InputFlags CPlayerInputManager::GetCurrentFrameInput() const
{
    return m_currentFrameInput;
}

bool CPlayerInputManager::HasInput(InputFlags flags) const
{
    return (m_currentFrameInput & (uint32_t)flags) != 0;
}

bool CPlayerInputManager::HasAllInputs(InputFlags flags) const
{
    return (m_currentFrameInput & (uint32_t)flags) == (uint32_t)flags;
}

bool CPlayerInputManager::HasAnyInput(InputFlags flags) const
{
    return (m_currentFrameInput & (uint32_t)flags) != 0;
}

int CPlayerInputManager::GetHorizontalInput() const
{
    // 비트 값들 확인
    uint32_t leftFlag = (uint32_t)INPUT_TYPE::MOVE_LEFT;
    uint32_t rightFlag = (uint32_t)INPUT_TYPE::MOVE_RIGHT;

    // HasInput 결과 확인
    bool left = HasInput(leftFlag);
    bool right = HasInput(rightFlag);

    if (left && !right) return -1;
    if (right && !left) return 1;
    return 0;
}

int CPlayerInputManager::GetVerticalInput() const
{
    bool up = HasInput((uint32_t)INPUT_TYPE::MOVE_UP);
    bool down = HasInput((uint32_t)INPUT_TYPE::MOVE_DOWN);

    if (up && !down) return 1;
    if (down && !up) return -1;
    return 0;
}

// === 입력 수집 함수들 ===

void CPlayerInputManager::CollectKeyboardInput()
{
    // === 방향 키 입력 ===
    if (KEY_HOLD(KEY::LEFT))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::MOVE_LEFT;
        m_bLeftCurrentlyPressed = true;
    }
    else
    {
        m_bLeftCurrentlyPressed = false;
    }

    if (KEY_HOLD(KEY::RIGHT))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::MOVE_RIGHT;
        m_bRightCurrentlyPressed = true;
    }
    else
    {
        m_bRightCurrentlyPressed = false;
    }

    if (KEY_HOLD(KEY::UP))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::MOVE_UP;
    }

    if (KEY_HOLD(KEY::DOWN))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::MOVE_DOWN;
    }

    // === 액션 키 입력 ===

    // Z키 (점프 - TAP, HOLD, AWAY 구분)
    if (KEY_TAP(KEY::Z))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::JUMP;
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::JUMP_TAP;
    }

    if (KEY_HOLD(KEY::Z))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::JUMP;
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::JUMP_HOLD;
        m_bJumpCurrentlyPressed = true;

        // 점프 홀드 시간 누적
        m_fJumpHoldTime += CTimeMgr::GetInst()->GetfDT();
        if (m_fJumpHoldTime > m_fMaxJumpHoldTime)
        {
            m_fJumpHoldTime = m_fMaxJumpHoldTime;
        }
    }
    else
    {
        m_bJumpCurrentlyPressed = false;
        m_fJumpHoldTime = 0.0f; // 키를 떼면 리셋
    }

    if (KEY_AWAY(KEY::Z))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::JUMP_AWAY;
    }

    // X키 (액션 - TAP, HOLD, AWAY 구분)
    if (KEY_TAP(KEY::X))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::ACTION;
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::ACTION_TAP;
    }

    if (KEY_HOLD(KEY::X))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::ACTION;
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::ACTION_HOLD;
        m_bActionCurrentlyPressed = true;
    }
    else
    {
        m_bActionCurrentlyPressed = false;
    }

    if (KEY_AWAY(KEY::X))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::ACTION_AWAY;
    }

    // BACKSPACE키 (능력 버리기)
    if (KEY_TAP(KEY::BACK))
    {
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::DROP_ABILITY;
        m_currentFrameInput |= (uint32_t)INPUT_TYPE::DROP_ABILITY_TAP;
    }
}

void CPlayerInputManager::ProcessDoubleTap()
{
    // Movement 스타일 더블탭 감지
    CheckMovementStyleDoubleTap();
}

void CPlayerInputManager::CheckMovementStyleDoubleTap()
{
    bool leftPressed = HasInput((uint32_t)INPUT_TYPE::MOVE_LEFT);
    bool rightPressed = HasInput((uint32_t)INPUT_TYPE::MOVE_RIGHT);

    int currentDirection = 0;
    if (leftPressed && !rightPressed) currentDirection = -1;
    else if (rightPressed && !leftPressed) currentDirection = 1;

    // 왼쪽 더블탭 체크
    if (currentDirection == -1)
    {
        // 감속 중이고, 같은 방향(왼쪽) 입력이면 더블탭
        if (m_bLeftDecelerating && m_iLastMoveDirection == -1)
        {
            m_currentFrameInput |= (uint32_t)INPUT_TYPE::DOUBLE_TAP_LEFT;
            m_bLeftDecelerating = false;
        }
        m_bRightDecelerating = false; // 다른 방향 감속 취소
    }
    // 오른쪽 더블탭 체크
    else if (currentDirection == 1)
    {
        // 감속 중이고, 같은 방향(오른쪽) 입력이면 더블탭
        if (m_bRightDecelerating && m_iLastMoveDirection == 1)
        {
            m_currentFrameInput |= (uint32_t)INPUT_TYPE::DOUBLE_TAP_RIGHT;
            m_bRightDecelerating = false;
        }
        m_bLeftDecelerating = false; // 다른 방향 감속 취소
    }
    // 입력이 없으면 감속 시작
    else if (currentDirection == 0)
    {
        if (m_iLastMoveDirection == -1 && !m_bLeftDecelerating)
        {
            m_bLeftDecelerating = true;
        }
        else if (m_iLastMoveDirection == 1 && !m_bRightDecelerating)
        {
            m_bRightDecelerating = true;
        }
    }

    // 방향 업데이트
    if (currentDirection != 0)
    {
        m_iLastMoveDirection = currentDirection;
    }
}

void CPlayerInputManager::UpdateInputFlags()
{
    // 조합 입력들 설정
    if (HasInput((uint32_t)INPUT_TYPE::MOVE_LEFT) || HasInput((uint32_t)INPUT_TYPE::MOVE_RIGHT))
    {
        //m_currentFrameInput |= (uint32_t)INPUT_TYPE::MOVE_HORIZONTAL;
    }

    if (HasAnyInput((uint32_t)INPUT_TYPE::ANY_MOVEMENT))
    {
        // ANY_MOVEMENT는 이미 개별 입력들의 조합이므로 별도 처리 불필요
    }
}

float CPlayerInputManager::GetJumpHoldRatio() const
{
    if (m_fMaxJumpHoldTime <= 0.0f) return 0.0f;
    return m_fJumpHoldTime / m_fMaxJumpHoldTime;
}