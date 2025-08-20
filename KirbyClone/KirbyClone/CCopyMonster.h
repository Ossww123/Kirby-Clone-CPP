#pragma once
#include "CBasicMonster.h"

class CCopyMonster : public CBasicMonster
{
public:
    CCopyMonster();
    virtual ~CCopyMonster();

public:
    // === 공격 시스템 (final로 하위 클래스에서 변경 불가) ===
    bool HasAttack() const override final { return true; }

    // === 순수 가상 함수 (하위 클래스에서 반드시 구현) ===
    virtual void Attack() = 0;                          // 공격 패턴
    virtual COPY_ABILITY GetCopyAbility() const = 0;    // 카피 능력 반환

public:
    // === 공격 관련 인터페이스 ===
    bool CanAttack() const;                             // 공격 가능 여부
    void StartAttack();                                 // 공격 시작
    void EndAttack();                                   // 공격 종료

    // === 카피 능력 관련 ===
    virtual void OnCopyAbilityGiven();                  // 카피 능력 제공 시 처리

public:
    // === 공격 상태 확인 ===
    bool IsAttacking() const { return m_bAttacking; }
    float GetAttackCooldown() const { return m_fAttackCooldown; }

protected:
    // === 공격 시스템 관리 ===
    void UpdateAttackCooldown();                        // 공격 쿨타임 업데이트
    void CheckAttackCondition();                        // 공격 조건 체크
    void SetAttackCooldown(float _fCooldown) { m_fMaxAttackCooldown = _fCooldown; }

    // === 상태 업데이트 오버라이드 ===
    void UpdateWalk() override;
    void UpdateAttackReady() override;
    void UpdateAttack() override;

protected:
    // === 공격 관련 헬퍼 함수들 ===
    bool IsPlayerInRange() const;                       // 플레이어가 공격 범위 내에 있는지
    Vec2 GetPlayerDirection() const;                    // 플레이어 방향 벡터
    void AimAtPlayer();                                 // 플레이어 조준

private:
    // === 공격 시스템 내부 처리 ===
    void ProcessAttackLogic();                          // 공격 로직 처리

protected:
    // === 공격 관련 변수들 ===
    bool    m_bAttacking;           // 공격 중 상태
    float   m_fAttackCooldown;      // 현재 공격 쿨타임
    float   m_fMaxAttackCooldown;   // 최대 공격 쿨타임
    float   m_fAttackRange;         // 공격 범위
    float   m_fAttackReadyTime;     // 공격 준비 시간
    float   m_fAttackDuration;      // 공격 지속 시간

    // === 플레이어 추적 ===
    Vec2    m_vPlayerPos;           // 플레이어 위치
    bool    m_bPlayerDetected;      // 플레이어 감지 여부
};