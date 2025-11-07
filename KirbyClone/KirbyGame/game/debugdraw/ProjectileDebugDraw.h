#pragma once
//
// Responsibility: Debug-draw adapter for Projectile (bounds).
// Non-Goals:      Regular rendering, materials, pooling.
// Call-Context:   Main thread during debug pass.
//

namespace engine { class IDebugDraw; }
namespace game { class Projectile; }

namespace game {
    struct ProjectileDebugDraw {
        static void Draw ( const Projectile& p , engine::IDebugDraw* dbg , int ox , int oy );
    };
}
