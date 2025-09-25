#include "gamePCH.h"
#include "CBrontoBurt.h"
#include "CRigidBody.h"
#include "CAnimator.h"
#include "CTimeMgr.h"
#include <cmath>

CBrontoBurt::CBrontoBurt()
    : CBasicMonster([] {
    BasicMonsterConfig c{};
    // 이동
    c.walkSpeed = 0.f;
    c.flySpeed = 90.f;
    c.chaseSpeedMul = 1.15f;
    c.canFly = true;        // 비행형
    // 감지/공격
    c.canAttack = false;
    c.sightRange = 220.f;
    c.attackRange = 0.f;
    c.attackCooldown = 0.f;
    // 흡입/능력
    c.inhalable = true;
    c.abilityGift = AbilityGift::None;
    return c;
        }())
{
    LoadAnimationsFromFile(L"bin/content/animation/BrontoBurt.json");

    // 비행형은 중력 OFF 권장
    if (auto* rb = GetRigidBody())
        rb->SetUseGravity(false);
}

void CBrontoBurt::Move()
{
    float speed = GetConfig().flySpeed * (m_bChasing ? GetConfig().chaseSpeedMul : 1.f);
    if (speed != 0.f) MoveHorizontal(speed);

    if (auto* rb = GetRigidBody())
    {
        m_fWaveTime += CTimeMgr::GetInst()->GetfDT() * m_fWaveSpeed;
        float vy = std::sin(m_fWaveTime) * m_fWaveVelAmp;
        Vec2 v = rb->GetVelocity();
        v.y = vy; // 중력 OFF 전제
        rb->SetVelocity(v);
    }
}


// 필요시 애니 이름이 다른 경우만 열어서 커스텀
// void CBrontoBurt::SetupAnimationMapping()
// {
//     ClearAnimMap();
//     MapAnims({
//         { MONSTER_STATE::FLY,            L"BB_FLY" },
//         { MONSTER_STATE::DAMAGE,         L"BB_HURT" },
//         { MONSTER_STATE::BEING_INHALED,  L"BB_INHALED" },
//     });
// }
