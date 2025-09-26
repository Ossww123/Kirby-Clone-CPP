#pragma once
#include "CMonster.h"
#include <string>

// 보스 공격 유형(필요 시 자유 확장)
enum class BossAttackKind : unsigned char {
    None, Projectile, Charge, Jump, Summon
};

// 보스 전역 설정 (페이즈 없음)
struct BossConfig {
    float moveSpeed = 60.f;   // 기본 좌우 이동 속도
    float attackCooldown = 1.5f;   // 공격 간 대기(초)
    bool  useGravity = false;  // 대부분 보스는 중력 off
    bool  canTurnOnWall = false;  // 벽 히트 시 자동 턴 허용 여부
};

class CBoss : public CMonster
{
public:
    explicit CBoss(const BossConfig& cfg = BossConfig{});
    ~CBoss() override = default;

    void Update() override;

    float GetHPRatio() const { return m_maxHp > 0 ? (float)m_hp / (float)m_maxHp : 0.f; }
    bool  IsDefeated() const { return m_hp <= 0; }

    void StartBossEvent();         // 연출/카메라 락 등은 나중에 채움
    void TakeBossDamage(int dmg);  // DamageSystem에서 호출

protected:
    // CMonster 필수 구현
    void Move() override;                      // 기본 이동(공격 중 정지)
    bool CanBeInhaled() const override { return false; }
    bool IsBeingInhaled() const override { return false; }
    bool HasAttack() const override { return true; }

    void SetupAnimationMapping() override;     // 보스용 기본 애니 매핑

    // ===== 파생 보스 훅 =====
    // 다음 공격을 시작해야 하면 공격 종류 반환(없으면 None)
    virtual BossAttackKind ChooseNextAttack();

    // READY/ATTACK 전이 지점(한 번만 호출)
    virtual void OnAttackReadyEnter(BossAttackKind kind) { (void)kind; }
    virtual void OnAttackEnter(BossAttackKind kind) { (void)kind; }
    virtual void OnAttackTick(BossAttackKind kind, float stateTime) { (void)kind; (void)stateTime; }
    virtual void OnAttackExit(BossAttackKind kind) { (void)kind; }

    // (선택) 애니 이름 빠르게 교체
    void SetAnimNames(const std::wstring& idle, const std::wstring& walk,
        const std::wstring& ready, const std::wstring& attack,
        const std::wstring& hurt = L"DAMAGE");

    const BossConfig& GetBossConfig() const { return m_cfg; }

    int m_hp = 64;   // 임시 기본값
    int m_maxHp = 64;

private:
    void UpdateBossAI(float dt);     // 쿨다운/공격 시작 판단
    void DispatchAttackHooks();      // 상태 전이 감지 → 훅 호출

private:
    BossConfig     m_cfg{};

    // 공격 관리
    float          m_attackCooldownLeft{ 0.f };
    BossAttackKind m_plannedAttack{ BossAttackKind::None }; // READY에 들어갈 때 확정
    BossAttackKind m_activeAttack{ BossAttackKind::None }; // ATTACK 중 진행 중

    // 상태 전이 감지용
    MONSTER_STATE  m_lastState{ MONSTER_STATE::END };

    // 보스 전용 애니 이름(선택)
    std::wstring m_animIdle = L"BOSS_IDLE";
    std::wstring m_animWalk = L"BOSS_WALK";
    std::wstring m_animReady = L"BOSS_ATTACK_READY";
    std::wstring m_animAttack = L"BOSS_ATTACK";
    std::wstring m_animHurt = L"DAMAGE";
};
