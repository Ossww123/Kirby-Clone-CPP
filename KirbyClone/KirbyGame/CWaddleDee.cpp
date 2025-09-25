#include "gamePCH.h"
#include "CWaddleDee.h"
#include "CCollider.h"
#include "CAnimator.h"

CWaddleDee::CWaddleDee()
    : CBasicMonster([] {
    BasicMonsterConfig c{};          // 기본값으로 채워진 뒤,
    // 이동
    c.walkSpeed = 70.f;
    c.flySpeed = 0.f;
    c.chaseSpeedMul = 1.15f;
    c.canFly = false;
    // 감지/공격
    c.canAttack = false;       // 기본 접촉 대미지형
    c.sightRange = 180.f;
    c.attackRange = 0.f;
    c.attackCooldown = 0.f;
    // 흡입/능력
    c.inhalable = true;
    c.abilityGift = AbilityGift::None;
    return c;                         // 완성된 config 반환
        }())
{
    // 애니메이션 데이터 로드
    LoadAnimationsFromFile(L"bin/content/animation/WaddleDee.json");

    // 필요하면 콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(48.f, 48.f));
}

// 필요시 애니 이름이 다른 경우만 열어서 커스텀
// void CWaddleDee::SetupAnimationMapping()
// {
//     ClearAnimMap();
//     MapAnims({
//         { MONSTER_STATE::IDLE,           L"WD_IDLE" },
//         { MONSTER_STATE::WALK,           L"WD_WALK" },
//         { MONSTER_STATE::TURN,           L"WD_TURN" },
//         { MONSTER_STATE::DAMAGE,         L"WD_HURT" },
//         { MONSTER_STATE::BEING_INHALED,  L"WD_INHALED" },
//     });
// }
