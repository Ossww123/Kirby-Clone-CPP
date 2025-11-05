#pragma once
#include "game/Ability.h"

namespace game {
    struct CombatTarget {
        int  id = -1;
        RECT aabb{ 0,0,0,0 };
        bool alive = true;
        bool isPlayer = false;

        // ---- Inhale support ----
        bool    inhalable{ false };
        Ability abilityGift{ Ability::None };
    };
}
