#include "gamePCH.h"
#include "CWaddleDoo.h"
#include "CAnimator.h"
#include "CCollider.h"

CWaddleDoo::CWaddleDoo()
    : CBasicMonster([] {
    BasicMonsterConfig c{};
    // 이동
    c.walkSpeed = 75.f;
    c.flySpeed = 0.f;
    c.chaseSpeedMul = 1.15f;
    c.canFly = false;
    // 감지/공격 (빔은 중거리)
    c.canAttack = true;
    c.sightRange = 260.f;
    c.attackRange = 120.f;
    c.attackCooldown = 1.8f;
    // 흡입/능력
    c.inhalable = true;
    c.abilityGift = AbilityGift::Beam;
    return c;
        }())
{
    LoadAnimationsFromFile(L"bin/content/animation/WaddleDoo.json");

    // 필요하면 콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(54.f, 54.f));
}

// 필요시 다른 이름의 애니를 쓰면 커스텀
// void CWaddleDoo::SetupAnimationMapping()
// {
//     ClearAnimMap();
//     MapAnims({
//         { MONSTER_STATE::IDLE,           L"DOO_IDLE" },
//         { MONSTER_STATE::WALK,           L"DOO_WALK" },
//         { MONSTER_STATE::ATTACK_READY,   L"DOO_BEAM_READY" },
//         { MONSTER_STATE::ATTACK,         L"DOO_BEAM" },
//         { MONSTER_STATE::DAMAGE,         L"DOO_HURT" },
//         { MONSTER_STATE::BEING_INHALED,  L"DOO_INHALED" },
//     });
// }
