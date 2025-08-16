#include "pch.h"
#include "CPlayerMovement.h"
#include "CPlayerInputManager.h"
#include "CPlayer.h"
#include "CRigidBody.h"
#include "CKeyMgr.h"
#include "CTimeMgr.h"

CPlayerMovement::CPlayerMovement(CPlayer* _pOwner) :
    m_pOwner(_pOwner),
    m_pInputManager(nullptr),
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
    m_fCrouchDeceleration(1200.f)
{
}

CPlayerMovement::~CPlayerMovement()
{
}

void CPlayerMovement::Init()
{

}

void CPlayerMovement::Update()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    // 흡입 중이면 이동 제한
    if (m_pOwner && m_pOwner->IsInhaling())
    {
        StopMovement();
        return;
    }

    // === 물리적 이동만 처리 ===
    ApplyCurrentMovement();

    // === 방향 및 상태 업데이트 ===
    UpdateDirection();
    UpdateMovementState();
}

// === 새로 추가: 현재 상태에 따른 물리적 이동 적용 ===
void CPlayerMovement::ApplyCurrentMovement()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!m_pOwner || !pRigidBody)
    {
        return;
    }

    PLAYER_STATE currentState = m_pOwner->GetCurrentState();

    // 특정 상태에서는 이동 불가
    if (currentState == PLAYER_STATE::CROUCH ||
        currentState == PLAYER_STATE::SLIDE ||
        currentState == PLAYER_STATE::SWALLOW ||
        currentState == PLAYER_STATE::EXHALE)
    {
        return;
    }

    // 이동 입력 처리
    ProcessMovementInput();
}

// === 입력 처리 함수들 ===

// === 이동 입력 처리 ===
void CPlayerMovement::ProcessMovementInput()
{
    if (!m_pInputManager)
    {
        ProcessMovementInputLegacy();
        return;
    }

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
    {
        return;
    }

    int currentMoveDir = m_pInputManager->GetHorizontalInput();
    m_bInputPressed = (currentMoveDir != 0);

    if (currentMoveDir != 0)
    {
        // 더블탭 체크
        if ((currentMoveDir == -1 && m_pInputManager->IsDoubleTapLeft()) ||
            (currentMoveDir == 1 && m_pInputManager->IsDoubleTapRight()))
        {
            m_bRunMode = true;
        }

        m_bIsDecelerating = false;
        ResetDoubleTapState();

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;

        // 입에 물고 있으면 속도 감소
        if (m_pOwner && m_pOwner->HasMouthful())
            fCurrentSpeed *= 0.7f;

        float finalVelocity = fCurrentSpeed * currentMoveDir;

        pRigidBody->SetVelocityX(finalVelocity);
    }
    else
    {
        // 입력이 없으면 감속 시작
        if (!m_bIsDecelerating)
        {
            StartDeceleration(m_iLastMoveDir);
        }
        ApplyDeceleration();
    }

    // 방향 업데이트
    m_iLastMoveDir = currentMoveDir;
}

void CPlayerMovement::ProcessMovementInputLegacy()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    int currentMoveDir = 0;
    m_bInputPressed = false;

    if (KEY_HOLD(KEY::LEFT))
    {
        currentMoveDir = -1;
        m_bInputPressed = true;

        if (CheckDoubleTap(currentMoveDir))
        {
            m_bRunMode = true;
        }

        m_bIsDecelerating = false;
        ResetDoubleTapState();

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;

        if (m_pOwner && m_pOwner->HasMouthful())
            fCurrentSpeed *= 0.7f;

        pRigidBody->SetVelocityX(-fCurrentSpeed);
    }
    else if (KEY_HOLD(KEY::RIGHT))
    {
        currentMoveDir = 1;
        m_bInputPressed = true;

        if (CheckDoubleTap(currentMoveDir))
        {
            m_bRunMode = true;
        }

        m_bIsDecelerating = false;
        ResetDoubleTapState();

        float fCurrentSpeed = m_bRunMode ? m_fRunSpeed : m_fSpeed;

        if (m_pOwner && m_pOwner->HasMouthful())
            fCurrentSpeed *= 0.7f;

        pRigidBody->SetVelocityX(fCurrentSpeed);
    }
    else
    {
        if (!m_bIsDecelerating)
        {
            StartDeceleration(m_iLastMoveDir);
        }
        ApplyDeceleration();
    }

    m_iLastMoveDir = currentMoveDir;
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
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    float currentSpeedX = pRigidBody->GetVelocity().x;

    // 이미 정지 상태면 감속 완료
    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        pRigidBody->SetVelocityX(0.f);
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
        pRigidBody->SetVelocityX(newSpeedX);
    }
    else if (currentSpeedX < 0)
    {
        float newSpeedX = currentSpeedX + decelAmount;
        if (newSpeedX > -m_fMinMovingSpeed)
            newSpeedX = 0;
        pRigidBody->SetVelocityX(newSpeedX);
    }
}

// === 크라우치 상태 감속 처리 ===
void CPlayerMovement::ApplyCrouchDeceleration()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    float currentSpeedX = pRigidBody->GetVelocity().x;

    // 이미 정지 상태면 감속 완료
    if (abs(currentSpeedX) <= m_fMinMovingSpeed)
    {
        pRigidBody->SetVelocityX(0.f);
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
        pRigidBody->SetVelocityX(newSpeedX);
    }
    else if (currentSpeedX < 0)
    {
        float newSpeedX = currentSpeedX + decelAmount;
        if (newSpeedX > -m_fMinMovingSpeed)
            newSpeedX = 0;
        pRigidBody->SetVelocityX(newSpeedX);
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

    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return;

    // 점프 실행
    pRigidBody->SetVelocityY(-m_fJumpPower);
    pRigidBody->SetGround(false);
}

bool CPlayerMovement::CanJump() const
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return false;

    // 땅에 있을 때만 점프 가능
    return pRigidBody->IsGround();
}

// === 속도 직접 제어 (특수 상황용) ===

void CPlayerMovement::StopMovement()
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetVelocityX(0.f);
    }
    m_bInputPressed = false;
    m_bIsDecelerating = false;
    ResetDoubleTapState();
}

void CPlayerMovement::SetVelocityX(float _fVelX)
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (pRigidBody)
    {
        pRigidBody->SetVelocityX(_fVelX);
    }
}

// === Getter 함수들 ===

bool CPlayerMovement::IsActuallyMoving() const
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return false;

    float currentSpeedX = abs(pRigidBody->GetVelocity().x);
    return currentSpeedX > m_fMinMovingSpeed;
}

float CPlayerMovement::GetCurrentSpeed() const
{
    CRigidBody* pRigidBody = m_pOwner ? m_pOwner->GetRigidBody() : nullptr;
    if (!pRigidBody)
        return 0.f;

    return abs(pRigidBody->GetVelocity().x);
}