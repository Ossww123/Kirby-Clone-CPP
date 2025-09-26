#include "gamePCH.h"
#include "CBoss.h"
#include "CRigidBody.h"
#include "CTimeMgr.h"
#include <algorithm>

// ===== 생성/기본 설정 =================================================

CBoss::CBoss(const BossConfig& cfg)
    : CMonster()
    , m_cfg(cfg)
{
    if (auto* rb = GetRigidBody())
        rb->SetUseGravity(m_cfg.useGravity);

    // (필요 시) 보스 콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(64.f, 64.f));

    SetupAnimationMapping();
}

void CBoss::SetupAnimationMapping()
{
    ClearAnimMap();
    MapAnims({
        { MONSTER_STATE::IDLE,          m_animIdle.c_str()   },
        { MONSTER_STATE::WALK,          m_animWalk.c_str()   },
        { MONSTER_STATE::ATTACK_READY,  m_animReady.c_str()  },
        { MONSTER_STATE::ATTACK,        m_animAttack.c_str() },
        { MONSTER_STATE::DAMAGE,        m_animHurt.c_str()   },
        { MONSTER_STATE::BEING_INHALED, L"DAMAGE" }, // 보스는 흡입 X
        { MONSTER_STATE::TURN,          m_animWalk.c_str()   },
        { MONSTER_STATE::FLY,           m_animWalk.c_str()   },
        });
}

void CBoss::SetAnimNames(const std::wstring& idle, const std::wstring& walk,
    const std::wstring& ready, const std::wstring& attack,
    const std::wstring& hurt)
{
    m_animIdle = idle;
    m_animWalk = walk;
    m_animReady = ready;
    m_animAttack = attack;
    m_animHurt = hurt;
    SetupAnimationMapping();
}

// ===== 메인 루프 ======================================================

void CBoss::Update()
{
    const float dt = CTimeMgr::GetInst()->GetfDT();

    // 1) 쿨다운/공격 시작 판단 (상태 틱 이전에)
    UpdateBossAI(dt);

    // 2) 부모(CMonster) 상태 머신/물리/애니 업데이트
    CMonster::Update();

    // 3) READY/ATTACK 전이 감지 → 훅 호출
    DispatchAttackHooks();

    // 4) 보스는 벽 자동 턴을 막고 싶을 때
    if (!m_cfg.canTurnOnWall && GetCurrentState() == MONSTER_STATE::TURN) {
        ChangeState(MONSTER_STATE::WALK);
    }

    m_lastState = GetCurrentState();
}

void CBoss::StartBossEvent() {
    // TODO: 보스전 시작 연출이 필요해지면 구현
}

void CBoss::TakeBossDamage(int dmg) {
    dmg = (std::max)(0, dmg);
    m_hp = (std::max)(0, m_hp - dmg);
    // TODO: 피격 무적/히트스톱/사운드/패턴 분기 등 필요 시 나중에 확장
}

// ===== AI/공격 스케줄링 ==============================================

void CBoss::UpdateBossAI(float dt)
{
    m_attackCooldownLeft = max(0.f, m_attackCooldownLeft - dt);

    const MONSTER_STATE s = GetCurrentState();
    const bool busy =
        (s == MONSTER_STATE::ATTACK_READY || s == MONSTER_STATE::ATTACK ||
            s == MONSTER_STATE::DAMAGE || s == MONSTER_STATE::BEING_INHALED);

    if (busy || m_attackCooldownLeft > 0.f) return;

    // 파생 보스가 결정
    BossAttackKind next = ChooseNextAttack();
    if (next == BossAttackKind::None) return;

    m_plannedAttack = next;
    OnAttackReadyEnter(m_plannedAttack);
    ChangeState(MONSTER_STATE::ATTACK_READY);

    // 다음 공격까지의 기본 쿨다운
    m_attackCooldownLeft = m_cfg.attackCooldown;
}

void CBoss::DispatchAttackHooks()
{
    const MONSTER_STATE s = GetCurrentState();
    const MONSTER_STATE prev = m_lastState;

    // READY → ATTACK 진입
    if (prev != MONSTER_STATE::ATTACK && s == MONSTER_STATE::ATTACK) {
        m_activeAttack = m_plannedAttack;
        OnAttackEnter(m_activeAttack);
    }

    // ATTACK 틱
    if (s == MONSTER_STATE::ATTACK) {
        OnAttackTick(m_activeAttack, GetStateTime());
    }

    // ATTACK 종료 감지
    if (prev == MONSTER_STATE::ATTACK && s != MONSTER_STATE::ATTACK) {
        OnAttackExit(m_activeAttack);
        m_activeAttack = BossAttackKind::None;
        m_plannedAttack = BossAttackKind::None;
    }
}

// ===== 이동 ===========================================================

void CBoss::Move()
{
    // 공격 준비/공격 중엔 기본 이동 정지(패턴이 직접 이동 제어해도 됨)
    const MONSTER_STATE s = GetCurrentState();
    if (s == MONSTER_STATE::ATTACK_READY || s == MONSTER_STATE::ATTACK) {
        if (auto* rb = GetRigidBody()) rb->SetVelocityX(0.f);
        return;
    }

    if (m_cfg.moveSpeed != 0.f)
        MoveHorizontal(m_cfg.moveSpeed);
}

// ===== 기본 공격 선택(파생에서 재정의) ===============================

BossAttackKind CBoss::ChooseNextAttack()
{
    // 기본 구현: 아무 것도 하지 않음 → 공격 시작 안 함
    return BossAttackKind::None;
}
