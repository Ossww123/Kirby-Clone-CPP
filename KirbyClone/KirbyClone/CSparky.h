#pragma once
#include "CCopyMonster.h"

class CSparky : public CCopyMonster
{
public:
    CSparky();
    virtual ~CSparky();

public:
    // === 가상 함수 구현 ===
    void Move() override;                                           // 점프 + 전기 공격
    void Attack() override;                                         // 전기 공격
    COPY_ABILITY GetCopyAbility() const override { return COPY_ABILITY::SPARK; }

protected:
    // === 애니메이션 생성 구현 ===
    void CreateAnimations() override;

    // === 상태 업데이트 오버라이드 ===
    void UpdateWalk() override;

private:
    // === 스파키 전용 이동 ===
    void JumpMove();                                                // 점프 이동
    void UpdateJumpPattern();                                       // 점프 패턴 업데이트

    // === 스파키 전용 공격 ===
    void CreateElectricField();                                     // 전기장 생성
    void CreateElectricSpark();                                     // 전기 스파크 생성

private:
    // === 점프 관련 ===
    bool    m_bJumping;                                             // 점프 중 상태
    float   m_fJumpTimer;                                           // 점프 타이머
    float   m_fJumpInterval;                                        // 점프 간격
    float   m_fJumpForce;                                           // 점프 힘

    // === 공격 관련 ===
    bool    m_bElectricFieldCreated;                                // 전기장 생성 여부
};