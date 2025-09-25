#include "gamePCH.h"
#include "CHotHead.h"
#include "CAnimator.h"
#include "CCollider.h"

CHotHead::CHotHead()
    : CBasicMonster([] {
    BasicMonsterConfig c{};
    // 이동 (무거운 느낌으로 조금 느리게)
    c.walkSpeed = 60.f;
    c.flySpeed = 0.f;
    c.chaseSpeedMul = 1.1f;
    c.canFly = false;
    // 감지/공격 (근중거리 화염 분사)
    c.canAttack = true;
    c.sightRange = 220.f;
    c.attackRange = 90.f;
    c.attackCooldown = 1.4f;
    // 흡입/능력
    c.inhalable = true;
    c.abilityGift = AbilityGift::Fire;
    return c;
        }())
{
    LoadAnimationsFromFile(L"bin/content/animation/HotHead.json");

    // 필요하면 콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(56.f, 56.f));
}

// void CHotHead::SetupAnimationMapping()
// {
//     ClearAnimMap();
//     MapAnims({
//         { MONSTER_STATE::IDLE,           L"HH_IDLE" },
//         { MONSTER_STATE::WALK,           L"HH_WALK" },
//         { MONSTER_STATE::ATTACK_READY,   L"HH_FIRE_READY" },
//         { MONSTER_STATE::ATTACK,         L"HH_FIRE" },
//         { MONSTER_STATE::DAMAGE,         L"HH_HURT" },
//         { MONSTER_STATE::BEING_INHALED,  L"HH_INHALED" },
//     });
// }
