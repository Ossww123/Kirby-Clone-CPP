#pragma once
#include "AbilityTypes.h"
#include "CMonster.h"
#include <cmath>

// 전방 선언
class CObject;

struct BasicMonsterConfig
{
    // 이동
    float walkSpeed = 80.f;
    float flySpeed = 80.f;
    float chaseSpeedMul = 1.3f;
    bool  canFly = false;

    // 감지/공격
    bool  canAttack = false;
    float sightRange = 220.f;
    float attackRange = 60.f;
    float attackCooldown = 1.2f;

    // 흡입/능력
    bool         inhalable = true;
    AbilityGift  abilityGift = AbilityGift::None;
};

class CBasicMonster : public CMonster, public IInhalable
{
public:
    explicit CBasicMonster(const BasicMonsterConfig& cfg = {});
    ~CBasicMonster() override = default;

    // 생명주기
    void Update() override;

protected:
    // CMonster 필수 구현
    void Move() override;                     // 상태 기반 속도 의도
    bool CanBeInhaled() const override { return m_Cfg.inhalable; }
    bool IsBeingInhaled() const override {
        return GetCurrentState() == MONSTER_STATE::BEING_INHALED;
    }
    bool HasAttack() const override { return m_Cfg.canAttack; }

    void SetupAnimationMapping() override;

public:
    // 외부 제어
    void SetTarget(CObject* p) { m_pTarget = p; }
    void SetConfig(const BasicMonsterConfig& cfg) { m_Cfg = cfg; }
    const BasicMonsterConfig& GetConfig() const { return m_Cfg; }

    // IInhalable
    bool IsInhalable() const override { return m_Cfg.inhalable; }
    AbilitySourceToken GetAbilityToken() const override {
        return AbilitySourceToken{ m_Cfg.abilityGift, 1 };
    }
    void OnInhaledStart() override { ChangeState(MONSTER_STATE::BEING_INHALED); }
    void OnSwallowed()     override { SetDead(); }
    void OnSpatOut()       override { SetDead(); }

protected:
    // 간단 AI
    void  SenseAndDecide();
    bool  TryStartAttack();
    float DistanceToTarget() const;
    void  FaceToTargetX();

protected:
    BasicMonsterConfig m_Cfg{};
    CObject* m_pTarget{ nullptr };

    // 런타임 상태
    bool  m_bChasing{ false };
    float m_fAttackCooldownLeft{ 0.f };
};
