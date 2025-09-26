#pragma once

#include "EventDef.h"

class DamageSystem {
public:
    static void Init();
    static void Shutdown();
private:
    static size_t s_subPlayerDamage;
    static size_t s_subPlayerRecoil;
    static size_t s_subMonsterDamage;

    static void OnPlayerDamage(const tEvent& e);   // w: CKirby*, l: Vec2*
    static void OnPlayerRecoil(const tEvent& e);   // optional
    static void OnMonsterDamage(const tEvent& e);   // w: CMonster*, l: CProjectile*
};
