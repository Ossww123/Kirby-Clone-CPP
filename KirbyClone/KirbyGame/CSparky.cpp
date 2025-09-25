#include "gamePCH.h"
#include "CSparky.h"
#include "CAnimator.h"
#include "CCollider.h"

CSparky::CSparky()
    : CBasicMonster([] {
    BasicMonsterConfig c{};
    // 이동 (짧은 리치의 전기, 기본 보행)
    c.walkSpeed = 70.f;
    c.flySpeed = 0.f;
    c.chaseSpeedMul = 1.2f;
    c.canFly = false;
    // 감지/공격 (근거리 전기 방출)
    c.canAttack = true;
    c.sightRange = 200.f;
    c.attackRange = 60.f;
    c.attackCooldown = 1.0f;
    // 흡입/능력
    c.inhalable = true;
    c.abilityGift = AbilityGift::Spark;
    return c;
        }())
{
    LoadAnimationsFromFile(L"bin/content/animation/Sparky.json");

    // 필요하면 콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(52.f, 52.f));
}

// void CSparky::SetupAnimationMapping()
// {
//     ClearAnimMap();
//     MapAnims({
//         { MONSTER_STATE::IDLE,           L"SP_IDLE" },
//         { MONSTER_STATE::WALK,           L"SP_WALK" },
//         { MONSTER_STATE::ATTACK_READY,   L"SP_SPARK_READY" },
//         { MONSTER_STATE::ATTACK,         L"SP_SPARK" },
//         { MONSTER_STATE::DAMAGE,         L"SP_HURT" },
//         { MONSTER_STATE::BEING_INHALED,  L"SP_INHALED" },
//     });
// }
