#include "gamePCH.h"
#include "CBasicMonster.h"
#include "CObject.h"
#include "CRigidBody.h"
#include "CAnimator.h"
#include "CTimeMgr.h"

CBasicMonster::CBasicMonster(const BasicMonsterConfig& cfg)
    : CMonster()
    , m_Cfg(cfg)
{
    // 필요 시 여기서 애니메이션 로드
    // LoadAnimationsFromFile(L"data\\anim\\enemies\\<name>.json");
}

void CBasicMonster::Update()
{
    // 쿨다운 갱신
    const float dt = CTimeMgr::GetInst()->GetfDT();
    if (m_fAttackCooldownLeft > 0.f)
        m_fAttackCooldownLeft = max(0.f, m_fAttackCooldownLeft - dt);

    // 감지/의사결정
    SenseAndDecide();

    // 부모 로직(상태 틱→Move() 호출→컴포넌트 업데이트)
    CMonster::Update();
}

void CBasicMonster::Move()
{
    float speed = 0.f;

    switch (GetCurrentState())
    {
    case MONSTER_STATE::WALK:
        speed = m_Cfg.walkSpeed * (m_bChasing ? m_Cfg.chaseSpeedMul : 1.f);
        break;

    case MONSTER_STATE::FLY:
        speed = m_Cfg.flySpeed * (m_bChasing ? m_Cfg.chaseSpeedMul : 1.f);
        break;

    default:
        speed = 0.f;
        break;
    }

    if (speed != 0.f)
        MoveHorizontal(speed);
}

void CBasicMonster::SetupAnimationMapping()
{
    ClearAnimMap();
    MapAnims({
        { MONSTER_STATE::IDLE,           L"IDLE" },
        { MONSTER_STATE::WALK,           L"WALK" },
        { MONSTER_STATE::TURN,           L"TURN" },
        { MONSTER_STATE::FLY,            L"FLY" },
        { MONSTER_STATE::DAMAGE,         L"DAMAGE" },
        { MONSTER_STATE::BEING_INHALED,  L"INHALED" },
        { MONSTER_STATE::ATTACK_READY,   L"ATTACK_READY" },
        { MONSTER_STATE::ATTACK,         L"ATTACK" },
        });
}

void CBasicMonster::SenseAndDecide()
{
    const MONSTER_STATE s = GetCurrentState();

    // 피격/흡입/공격 중에는 의사결정 억제
    if (s == MONSTER_STATE::DAMAGE || s == MONSTER_STATE::BEING_INHALED ||
        s == MONSTER_STATE::ATTACK_READY || s == MONSTER_STATE::ATTACK)
    {
        m_bChasing = false;
        return;
    }

    // 비행형이면 WALK 상태를 FLY로 자동 교체 (현재 enum만 사용)
    if (m_Cfg.canFly && s == MONSTER_STATE::WALK)
        ChangeState(MONSTER_STATE::FLY);

    // 타깃이 없으면 기본 패턴 유지
    const float dist = DistanceToTarget();
    if (!m_pTarget || !std::isfinite(dist))
    {
        m_bChasing = false;
        return;
    }

    // 타깃 바라보기
    FaceToTargetX();

    // 공격 우선
    if (m_Cfg.canAttack && dist <= m_Cfg.attackRange)
    {
        if (TryStartAttack()) return;
    }

    // 추격 여부(상태는 그대로, 속도만 가속)
    m_bChasing = (dist <= m_Cfg.sightRange);
}

bool CBasicMonster::TryStartAttack()
{
    if (m_fAttackCooldownLeft > 0.f) return false;

    ChangeState(MONSTER_STATE::ATTACK_READY);
    m_fAttackCooldownLeft = m_Cfg.attackCooldown;
    return true;
}

float CBasicMonster::DistanceToTarget() const
{
    if (!m_pTarget) return std::numeric_limits<float>::infinity();
    const Vec2 a = GetPos();
    const Vec2 b = m_pTarget->GetPos();
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

void CBasicMonster::FaceToTargetX()
{
    if (!m_pTarget) return;
    const float dx = m_pTarget->GetPos().x - GetPos().x;
    const int wantDir = (dx >= 0.f) ? +1 : -1;
    if (wantDir != GetDirection())
        SetDirection(wantDir);
}
