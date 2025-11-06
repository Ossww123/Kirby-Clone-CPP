#pragma once
//
// Responsibility: Cross-system combat enums.
// Non-Goals:      Ownership rules beyond tagging.
// Call-Context:   Header-only.
//
namespace game {
    // Who fired the projectile (used by hit filtering outside)
    enum class ProjOwner : int { Player , Enemy };
}
