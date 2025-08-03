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
    m_fJumpPower(400.f),
    m_bRunMode(false),
    m_bFacingRight(true),
    m_iLastMoveDir(0),
    m_bDirectionChanged(false),
    m_fDeceleration(800.f),
    m_fMinMovingSpeed(10.f),
    m_bIsDecelerating(false),
    m_bWasMovingLastFrame(false),
    m_bInputPressed(false)
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

    // 달리기 모드 토글 (Shift 키)
    if (KEY_TAP(KEY::SHIFT))
    {
        m_bRunMode = !m_bRunMode;
    }

    // 흡입 중이면 이동 제한
    if (m_pOwner && m_pOwner->IsInhaling())
    {
        StopMovement();
        return;
    }

    // 점프 처리
    HandleJumpInput();

    // 이동 처리
    HandleMovementInput();

    // 방향 업데이트
    UpdateDirection();

    // 이동 상태 업데이트
    UpdateMovementState();
}

void CPlayerMovement::HandleMovementInput()
{
    if (!m_pRigidBody)
        return;

    int currentMoveDir = 0;
    m_bInputPressed = false;

    // 왼쪽 이동
    if (KEY_HOLD(KEY::LEFT))
    {
        currentMoveDir = -1;
        m_bInputPressed = true;
        m_bIsDecelerating = false;

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
        m_bIsDecelerating = false;

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
            m_bIsDecelerating = true;
        }
        ApplyDeceleration();
    }

    // 방향 업데이트
    m_iLastMoveDir = currentMoveDir;
}

void CPlayerMovement::HandleJumpInput()
{
    if (!m_pRigidBody)
        return;

    if (KEY_TAP(KEY::SPACE))
    {
        Jump();
    }
}

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

void CPlayerMovement::ApplyDeceleration()
{
    if (!m_pRigidBody)
        return;

    float currentSpeedX = m_pRigidBody->GetVelocity().x;

    // 이미 정지 상태면 종료
    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        m_pRigidBody->SetVelocityX(0.f);
        m_bIsDecelerating = false;
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

void CPlayerMovement::SetFacingDirection(bool _bRight)
{
    if (m_bFacingRight != _bRight)
    {
        m_bFacingRight = _bRight;
    }
}

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

void CPlayerMovement::StopMovement()
{
    if (m_pRigidBody)
    {
        m_pRigidBody->SetVelocityX(0.f);
    }
    m_bInputPressed = false;
    m_bIsDecelerating = false;
}

void CPlayerMovement::SetVelocityX(float _fVelX)
{
    if (m_pRigidBody)
    {
        m_pRigidBody->SetVelocityX(_fVelX);
    }
}