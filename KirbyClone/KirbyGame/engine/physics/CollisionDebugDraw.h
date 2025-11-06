#pragma once
//
// Responsibility: Adapter to draw CollisionSystem with D3D11DebugDraw.
// Non-Goals:      Physics logic; keep physics core free of renderer deps.
// Call-Context:   Render thread; RGBA8 colors.
//
#include <cstdint>
#include "engine/physics/Collision.h"

namespace engine { class D3D11DebugDraw; }

namespace engine::physics {

    void DebugDraw ( const CollisionSystem& sys ,
                   engine::D3D11DebugDraw& dbg ,
                   int ox , int oy ,
                   std::uint32_t solidRGBA ,
                   std::uint32_t oneWayRGBA );

} // namespace engine::physics
