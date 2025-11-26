//
// Responsibility: Compile-time game constants (resolution, scale, world unit sizes).
// Non-Goals:      Runtime tuning, user config, file I/O.
// Call-Context:   Header-only; safe to include from engine/game code.
//
#pragma once

namespace game {

    // Integer zoom scale (render upscaling factor)
    inline constexpr int SCALE = 4;

    // Base (original) resolution in logical pixels
    inline constexpr int O_W = 240;
    inline constexpr int O_H = 160;

    // World base sizes (original, unscaled)
    inline constexpr int O_TILE_PX = 16; // tile size in world units
    inline constexpr int O_PLAYER_COLL_PX = 14; // Kirby AABB in world units
    inline constexpr int O_ENEMY_COLL_PX = 14; // default enemy AABB in world units

    // Derived client size (for window creation)
    inline constexpr int CLIENT_W = O_W * SCALE;
    inline constexpr int CLIENT_H = O_H * SCALE;

    // Scaled world sizes
    inline constexpr int TILE_PX = O_TILE_PX * SCALE;
    inline constexpr int GRID_PX = TILE_PX;                 // alias (avoid drift)
    inline constexpr int PLAYER_COLL_PX = O_PLAYER_COLL_PX * SCALE; // Kirby AABB (scaled)
    inline constexpr int ENEMY_COLL_PX = O_ENEMY_COLL_PX * SCALE; // default enemy AABB (scaled)


    inline constexpr int DEFAULT_LIVES = 2;

} // namespace game
