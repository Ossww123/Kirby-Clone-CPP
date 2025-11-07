//
// Responsibility: Lightweight hit/target descriptor shared by combat systems.
// Non-Goals:      Ownership or behavior logic; no collision math or lifetime management.
// Call-Context:   Engine/game layer; header-only POD, no Win32 types (uses IntRect).
//

#pragma once
#include "engine/util/Types.h"
#include "game/combat/Ability.h"

namespace game {

    struct CombatTarget {
        int id{ -1 };                           // object id/handle; -1 = invalid
        engine::IntRect aabb{ 0,0,0,0 };        // world-space AABB in pixels
        bool alive{ true };                     // eligible for damage/interaction
        bool isPlayer{ false };                 // player filter flag

        // Inhale / ability gift
        bool    inhalable{ false };             // can be inhaled by player
        Ability abilityGift{ Ability::None };   // ability granted on inhale/defeat
    };

} // namespace game
