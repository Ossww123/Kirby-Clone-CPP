#include "pch.h"
#include "CGordo.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"

CGordo::CGordo()
    : m_eMoveType(GORDO_MOVE_TYPE::HORIZONTAL)
    , m_vMoveDirection(Vec2(1.f, 0.f))
    , m_fMoveRange(200.f)
    , m_vStartPos(Vec2(0.f, 0.f))
    , m_vMinBound(Vec2(0.f, 0.f))
    , m_vMaxBound(Vec2(0.f, 0.f))
    , m_vCircularCenter(Vec2(0.f, 0.f))
    , m_fCircularRadius(100.f)
    , m_fCircularAngle(0.f)
    , m_fCircularSpeed(1.f)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_GORDOS);

    // 고르도 전용 설정
    m_fSpeed = 120.f;                       // 빠른 이동

    // 무적이므로 중력 비활성화 (자유롭게 이동)
    GetRigidBody()->SetUseGravity(false);

    // 시작 위치 저장
    m_vStartPos = GetPos();

    // 경계 설정 (시작점 기준 ±범위)
    m_vMinBound = Vec2(m_vStartPos.x - m_fMoveRange, m_vStartPos.y - m_fMoveRange);
    m_vMaxBound = Vec2(m_vStartPos.x + m_fMoveRange, m_vStartPos.y + m_fMoveRange);

    // 원형 이동 중심점 설정
    m_vCircularCenter = m_vStartPos;

    // 애니메이션 생성
    
    // 초기 상태 설정 (고르도는 계속 이동)
    ChangeState(MONSTER_STATE::FLY);
}

CGordo::~CGordo()
{
    // 상위 클래스에서 정리
}

void CGordo::Move()
{
    // 이동 타입에 따른 이동 패턴 실행
    switch (m_eMoveType)
    {
    case GORDO_MOVE_TYPE::HORIZONTAL:
        MoveHorizontal();
        break;
    case GORDO_MOVE_TYPE::VERTICAL:
        MoveVertical();
        break;
    case GORDO_MOVE_TYPE::DIAGONAL:
        MoveDiagonal();
        break;
    case GORDO_MOVE_TYPE::CIRCULAR:
        MoveCircular();
        break;
    }

    // 경계 충돌 처리
    HandleBoundaryCollision();
}

void CGordo::SetupAnimationMapping()
{
}

void CGordo::MoveHorizontal()
{
    // 좌우 직선 이동
    Vec2 currentPos = GetPos();
    float newX = currentPos.x + (m_fSpeed * m_vMoveDirection.x * CTimeMgr::GetInst()->GetfDT());

    SetPos(Vec2(newX, currentPos.y));

    // 리지드바디 속도 설정
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2(m_fSpeed * m_vMoveDirection.x, 0.f));
    }
}

void CGordo::MoveVertical()
{
    // 상하 직선 이동
    Vec2 currentPos = GetPos();
    float newY = currentPos.y + (m_fSpeed * m_vMoveDirection.y * CTimeMgr::GetInst()->GetfDT());

    SetPos(Vec2(currentPos.x, newY));

    // 리지드바디 속도 설정
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2(0.f, m_fSpeed * m_vMoveDirection.y));
    }
}

void CGordo::MoveDiagonal()
{
    // 대각선 이동
    Vec2 currentPos = GetPos();
    Vec2 newPos = currentPos + (m_vMoveDirection * m_fSpeed * CTimeMgr::GetInst()->GetfDT());

    SetPos(newPos);

    // 리지드바디 속도 설정
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocity(m_vMoveDirection * m_fSpeed);
    }
}

void CGordo::MoveCircular()
{
    // 원형 이동
    m_fCircularAngle += m_fCircularSpeed * CTimeMgr::GetInst()->GetfDT();

    // 각도 정규화
    if (m_fCircularAngle >= 2 * 3.14159f)
        m_fCircularAngle -= 2 * 3.14159f;

    // 원형 좌표 계산
    float x = m_vCircularCenter.x + cosf(m_fCircularAngle) * m_fCircularRadius;
    float y = m_vCircularCenter.y + sinf(m_fCircularAngle) * m_fCircularRadius;

    SetPos(Vec2(x, y));

    // 원형 이동의 속도 벡터 계산 (접선 방향)
    Vec2 velocity = Vec2(-sinf(m_fCircularAngle), cosf(m_fCircularAngle)) * m_fSpeed;

    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocity(velocity);
    }
}

void CGordo::HandleBoundaryCollision()
{
    Vec2 currentPos = GetPos();
    bool needReverse = false;

    // 이동 타입별 경계 체크
    switch (m_eMoveType)
    {
    case GORDO_MOVE_TYPE::HORIZONTAL:
        if (currentPos.x <= m_vMinBound.x || currentPos.x >= m_vMaxBound.x)
            needReverse = true;
        break;

    case GORDO_MOVE_TYPE::VERTICAL:
        if (currentPos.y <= m_vMinBound.y || currentPos.y >= m_vMaxBound.y)
            needReverse = true;
        break;

    case GORDO_MOVE_TYPE::DIAGONAL:
        if (currentPos.x <= m_vMinBound.x || currentPos.x >= m_vMaxBound.x ||
            currentPos.y <= m_vMinBound.y || currentPos.y >= m_vMaxBound.y)
            needReverse = true;
        break;

    case GORDO_MOVE_TYPE::CIRCULAR:
        // 원형 이동은 경계 체크 불필요
        break;
    }

    if (needReverse)
    {
        ReverseDirection();
    }
}

void CGordo::ReverseDirection()
{
    // 방향 반전
    m_vMoveDirection = m_vMoveDirection * -1.f;

    // 이동 방향에 따른 스프라이트 방향 변경
    if (m_eMoveType == GORDO_MOVE_TYPE::HORIZONTAL)
    {
        m_iDir *= -1;
    }
}