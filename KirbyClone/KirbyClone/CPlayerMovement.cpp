#include "pch.h"
#include "CPlayerMovement.h"
#include "CPlayer.h"
#include "CRigidBody.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"

CPlayerMovement::CPlayerMovement(CPlayer* _pOwner) :
    m_pOwner(_pOwner),
    m_pRigidBody(nullptr),
    m_fSpeed(150.f),
    m_fRunSpeed(250.f),
    m_fJumpPower(640.f),
    m_bRunMode(false),
    m_bFacingRight(true),
    m_iLastMoveDir(0),
    m_bDirectionChanged(false),
    m_fDeceleration(800.f),
    m_fMinMovingSpeed(10.f),
    m_bIsDecelerating(false),
    m_fDoubleTapWindow(0.3f),
    m_iDeceleratingDirection(0),
    m_bWasMovingLastFrame(false),
    m_bInputPressed(false),
    // === 크라우치 관련 초기화 ===
    m_bCrouchPressed(false),
    m_fCrouchDeceleration(1200.f)  // 일반 감속보다 빠르게
{
}

CPlayerMovement::~CPlayerMovement()
{
}

void CPlayerMovement::Init(CRigidBody* _pRigidBody)
{
    m_pRigidBody = _pRigidBody;
}

void CPlayerMovement::Update()
{
    if (!m_pRigidBody)
        return;

    // 흡입 중이면 이동 제한
    if (m_pOwner && m_pOwner->IsInhaling())
    {
        StopMovement();
        return;
    }

    // 크라우치 입력 처리
    HandleCrouchInput();

    // 점프 처리
    HandleJumpInput();

    // 일반 이동 처리
    if (m_pOwner && m_pOwner->GetCurrentState() != PLAYER_STATE::CROUCH)
    {
        HandleMovementInput();
    }

    // 방향 업데이트
    UpdateDirection();

    // 이동 상태 업데이트
    UpdateMovementState();
}

// === 입력 처리 함수들 ===

void CPlayerMovement::HandleMovementInput()
{
    if (!m_pRigidBody)
        return;

    // 크라우치 상태에서 X키(공격키) 입력 시 슬라이드
    if (KEY_TAP(KEY::X) && m_pOwner && m_pOwner->GetCurrentState() == PLAYER_STATE::CROUCH)
    {
        InitiateSlide();
        return;
    }

    int currentMoveDir = 0;
    m_bInputPressed = false;

    // 왼쪽 이동
    if (KEY_HOLD(KEY::LEFT))
    {
        currentMoveDir = -1;
        m_bInputPressed = true;

        // 더블탭 체크 - 이벤트 기반으로 상태 변경
        if (CheckDoubleTap(currentMoveDir))
        {
            m_bRunMode = true;
            // 상태도 즉시 업데이트 (이벤트 기반)
            if (m_pOwner)
            {
                PLAYER_STATE newState = m_pOwner->HasMouthful() ?
                    PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::RUN;
                m_pOwner->ChangeState(newState);
            }
        }

        m_bIsDecelerating = false;
        ResetDoubleTapState();

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;

        // 입에 물고 있으면 속도 감소
        if (m_pOwner && m_pOwner->HasMouthful())
            fCurrentSpeed *= 0.7f;

        m_pRigidBody->SetVelocityX(-fCurrentSpeed);
    }
    // 오른쪽 이동
    else if (KEY_HOLD(KEY::RIGHT))
    {
        currentMoveDir = 1;
        m_bInputPressed = true;

        // 더블탭 체크 - 이벤트 기반으로 상태 변경
        if (CheckDoubleTap(currentMoveDir))
        {
            m_bRunMode = true;
            // 상태도 즉시 업데이트 (이벤트 기반)
            if (m_pOwner)
            {
                PLAYER_STATE newState = m_pOwner->HasMouthful() ?
                    PLAYER_STATE::MOUTHFUL_RUN : PLAYER_STATE::RUN;
                m_pOwner->ChangeState(newState);
            }
        }

        m_bIsDecelerating = false;
        ResetDoubleTapState();

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;

        // 입에 물고 있으면 속도 감소
        if (m_pOwner && m_pOwner->HasMouthful())
            fCurrentSpeed *= 0.7f;

        m_pRigidBody->SetVelocityX(fCurrentSpeed);
    }
    // 입력이 없으면 감속 시작
    else
    {
        if (!m_bIsDecelerating)
        {
            StartDeceleration(m_iLastMoveDir);
        }
        ApplyDeceleration();
    }

    // 방향 업데이트
    m_iLastMoveDir = currentMoveDir;
}

void CPlayerMovement::HandleJumpInput()
{
    if (!m_pRigidBody || !m_pOwner)
        return;

    if (KEY_TAP(KEY::SPACE))
    {
        // 크라우치 상태에서는 슬라이드
        if (m_pOwner->GetCurrentState() == PLAYER_STATE::CROUCH)
        {
            InitiateSlide();
        }
        // 일반 상태에서는 점프
        else
        {
            Jump();
        }
    }
}

// === 크라우치 입력 처리 ===
void CPlayerMovement::HandleCrouchInput()
{
    if (!m_pOwner)
        return;

    // DOWN 키 입력 상태 체크
    m_bCrouchPressed = KEY_HOLD(KEY::DOWN);

    PLAYER_STATE currentState = m_pOwner->GetCurrentState();

    // 크라우치 진입 조건: 땅에 선 상태 + DOWN 키 + 머금은 상태 아님
    if (m_bCrouchPressed &&
        (currentState == PLAYER_STATE::IDLE ||
            currentState == PLAYER_STATE::WALK ||
            currentState == PLAYER_STATE::RUN) &&
        !m_pOwner->HasMouthful())
    {
        // 크라우치 상태로 전환
        m_pOwner->ChangeState(PLAYER_STATE::CROUCH);
    }
    // 크라우치 종료 조건: DOWN 키 해제
    else if (!m_bCrouchPressed && currentState == PLAYER_STATE::CROUCH)
    {
        // IDLE 상태로 복귀
        m_pOwner->ChangeState(PLAYER_STATE::IDLE);
    }

    // 크라우치 상태에서 방향 변경 처리
    if (currentState == PLAYER_STATE::CROUCH)
    {
        HandleCrouchDirectionInput();
    }
}

// === 방향 관리 함수들 ===

void CPlayerMovement::UpdateDirection()
{
    m_bDirectionChanged = false;

    // 이동 중일 때만 방향 업데이트
    if (m_iLastMoveDir != 0)
    {
        bool newFacingRight = (m_iLastMoveDir > 0);

        if (m_bFacingRight != newFacingRight)
        {
            SetFacingDirection(newFacingRight);
            m_bDirectionChanged = true;
        }
    }
}

void CPlayerMovement::UpdateMovementState()
{
    // 현재 실제로 움직이고 있는지 확인
    bool currentlyMoving = IsActuallyMoving();

    // 방향 변경 감지
    if (m_bWasMovingLastFrame != currentlyMoving)
    {
        m_bDirectionChanged = true;
    }
    else
    {
        m_bDirectionChanged = false;
    }

    m_bWasMovingLastFrame = currentlyMoving;
}

// === 감속 관리 함수들 ===

void CPlayerMovement::StartDeceleration(int _iDirection)
{
    m_bIsDecelerating = true;
    m_iDeceleratingDirection = _iDirection;
}

void CPlayerMovement::ApplyDeceleration()
{
    if (!m_pRigidBody)
        return;

    float currentSpeedX = m_pRigidBody->GetVelocity().x;

    // 이미 정지 상태면 감속 완료
    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        m_pRigidBody->SetVelocityX(0.f);
        m_bIsDecelerating = false;

        // 감속 완료 시 RUN 모드 해제 및 더블탭 상태 리셋
        m_bRunMode = false;
        ResetDoubleTapState();
        return;
    }

    // 감속 적용
    float deltaTime = CTimeMgr::GetInst()->GetfDT();
    float decelAmount = m_fDeceleration * deltaTime;

    if (currentSpeedX > 0)
    {
        float newSpeedX = currentSpeedX - decelAmount;
        if (newSpeedX < m_fMinMovingSpeed)
            newSpeedX = 0;
        m_pRigidBody->SetVelocityX(newSpeedX);
    }
    else if (currentSpeedX < 0)
    {
        float newSpeedX = currentSpeedX + decelAmount;
        if (newSpeedX > -m_fMinMovingSpeed)
            newSpeedX = 0;
        m_pRigidBody->SetVelocityX(newSpeedX);
    }
}

// === 크라우치 상태 감속 처리 ===
void CPlayerMovement::ApplyCrouchDeceleration()
{
    if (!m_pRigidBody)
        return;

    float currentSpeedX = m_pRigidBody->GetVelocity().x;

    // 이미 정지 상태면 감속 완료
    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        m_pRigidBody->SetVelocityX(0.f);
        return;
    }

    // 크라우치 감속 적용 (일반 감속보다 빠르게)
    float deltaTime = CTimeMgr::GetInst()->GetfDT();
    float decelAmount = m_fCrouchDeceleration * deltaTime;

    if (currentSpeedX > 0)
    {
        float newSpeedX = currentSpeedX - decelAmount;
        if (newSpeedX < m_fMinMovingSpeed)
            newSpeedX = 0;
        m_pRigidBody->SetVelocityX(newSpeedX);
    }
    else if (currentSpeedX < 0)
    {
        float newSpeedX = currentSpeedX + decelAmount;
        if (newSpeedX > -m_fMinMovingSpeed)
            newSpeedX = 0;
        m_pRigidBody->SetVelocityX(newSpeedX);
    }
}

// === 더블탭 시스템 함수들 ===

bool CPlayerMovement::CheckDoubleTap(int _iCurrentDir)
{
    // 감속 중이고, 같은 방향 입력이고, RUN 모드가 아닐 때만 더블탭 인식
    if (m_bIsDecelerating &&
        _iCurrentDir == m_iDeceleratingDirection &&
        _iCurrentDir != 0 &&
        !m_bRunMode)
    {
        return true;
    }
    return false;
}

void CPlayerMovement::ResetDoubleTapState()
{
    m_iDeceleratingDirection = 0;
}

// === 방향 관련 ===

void CPlayerMovement::SetFacingDirection(bool _bRight)
{
    if (m_bFacingRight != _bRight)
    {
        m_bFacingRight = _bRight;
    }
}

// === 크라우치 상태에서 방향 변경 처리 ===
void CPlayerMovement::HandleCrouchDirectionInput()
{
    // 크라우치 상태에서는 이동하지 않고 방향만 변경
    if (KEY_HOLD(KEY::LEFT))
    {
        SetFacingDirection(false);  // 왼쪽 보기
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        SetFacingDirection(true);   // 오른쪽 보기
    }

    // 크라우치 상태에서 감속 적용
    ApplyCrouchDeceleration();
}

// === 점프 관련 ===

void CPlayerMovement::Jump()
{
    if (!CanJump())
        return;

    // 점프 실행
    m_pRigidBody->SetVelocityY(-m_fJumpPower);
    m_pRigidBody->SetGround(false);
}

bool CPlayerMovement::CanJump() const
{
    if (!m_pRigidBody)
        return false;

    // 땅에 있을 때만 점프 가능
    return m_pRigidBody->IsGround();
}

// === 슬라이드 관련 함수들 ===
bool CPlayerMovement::CanSlide() const
{
    if (!m_pOwner)
        return false;

    // 크라우치 상태이고 머금은 상태가 아닐 때만 슬라이드 가능
    return (m_pOwner->GetCurrentState() == PLAYER_STATE::CROUCH &&
        !m_pOwner->HasMouthful());
}

void CPlayerMovement::InitiateSlide()
{
    if (!CanSlide() || !m_pOwner)
        return;

    // 슬라이드 상태로 전환
    m_pOwner->ChangeState(PLAYER_STATE::SLIDE);
}

// === 속도 직접 제어 (특수 상황용) ===

void CPlayerMovement::StopMovement()
{
    if (m_pRigidBody)
    {
        m_pRigidBody->SetVelocityX(0.f);
    }
    m_bInputPressed = false;
    m_bIsDecelerating = false;
    ResetDoubleTapState();
}

void CPlayerMovement::SetVelocityX(float _fVelX)
{
    if (m_pRigidBody)
    {
        m_pRigidBody->SetVelocityX(_fVelX);
    }
}

// === Getter 함수들 ===

bool CPlayerMovement::IsActuallyMoving() const
{
    if (!m_pRigidBody)
        return false;

    float currentSpeedX = abs(m_pRigidBody->GetVelocity().x);
    return currentSpeedX > m_fMinMovingSpeed;
}

float CPlayerMovement::GetCurrentSpeed() const
{
    if (!m_pRigidBody)
        return 0.f;

    return abs(m_pRigidBody->GetVelocity().x);
}