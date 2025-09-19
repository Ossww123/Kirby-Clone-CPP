#pragma once
#include "CInvincibleMonster.h"

enum class GORDO_MOVE_TYPE
{
    HORIZONTAL,     // 좌우 이동
    VERTICAL,       // 상하 이동
    DIAGONAL,       // 대각선 이동
    CIRCULAR,       // 원형 이동
};

class CGordo : public CInvincibleMonster
{
public:
    CGordo();
    virtual ~CGordo();

public:
    // === 가상 함수 구현 ===
    void Move() override;                                   // 직선/원형 이동

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

public:
    // === 고르도 전용 설정 ===
    void SetMoveType(GORDO_MOVE_TYPE _eType) { m_eMoveType = _eType; }
    void SetMoveRange(float _fRange) { m_fMoveRange = _fRange; }
    void SetCircularCenter(Vec2 _vCenter) { m_vCircularCenter = _vCenter; }

private:
    // === 이동 패턴별 함수들 ===
    void MoveHorizontal();                                  // 좌우 이동
    void MoveVertical();                                    // 상하 이동
    void MoveDiagonal();                                    // 대각선 이동
    void MoveCircular();                                    // 원형 이동

    // === 경계 처리 ===
    void HandleBoundaryCollision();                         // 경계 충돌 처리
    void ReverseDirection();                                // 방향 반전

private:
    // === 이동 패턴 관련 ===
    GORDO_MOVE_TYPE m_eMoveType;                           // 이동 타입
    Vec2            m_vMoveDirection;                       // 이동 방향
    float           m_fMoveRange;                           // 이동 범위

    // === 경계 관련 ===
    Vec2            m_vStartPos;                            // 시작 위치
    Vec2            m_vMinBound;                            // 최소 경계
    Vec2            m_vMaxBound;                            // 최대 경계

    // === 원형 이동 관련 ===
    Vec2            m_vCircularCenter;                      // 원형 이동 중심점
    float           m_fCircularRadius;                      // 원형 이동 반지름
    float           m_fCircularAngle;                       // 현재 각도
    float           m_fCircularSpeed;                       // 각속도
};