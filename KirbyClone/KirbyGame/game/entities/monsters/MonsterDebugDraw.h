#pragma once
//
// Responsibility: Debug-draw adapter for Monster (bounds + hp bar).
// Non-Goals:      Regular rendering; animation/sprite policy.
// Call-Context:   Main thread during debug render pass.
//

namespace engine { class D3D11DebugDraw; }

namespace game {
    class Monster;

    struct MonsterDebugDraw {
        static void Draw ( const Monster& m , engine::D3D11DebugDraw* dbg , int ox , int oy );
    };
}
