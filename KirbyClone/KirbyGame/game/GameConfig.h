#pragma once
namespace game {
    // scale
    inline constexpr int SCALE = 4;

    // original resolution
    inline constexpr int O_W = 240;
    inline constexpr int O_H = 160;

    // World original sizes
    inline constexpr int O_TILE_PX          = 16;   // tile size in world units
    inline constexpr int O_PLAYER_COLL_PX   = 14;   // Kirby AABB in world units
    inline constexpr int O_ENEMY_COLL_PX    = 14;   // default enemy AABB in world units

    // Derived client size (for window creation)
    inline constexpr int CLIENT_W = O_W * SCALE;
    inline constexpr int CLIENT_H = O_H * SCALE;
    inline constexpr int TILE_PX = O_TILE_PX * SCALE;
    inline constexpr int GRID_PX = O_TILE_PX * SCALE;
    inline constexpr int PLAYER_COLL_PX = O_PLAYER_COLL_PX * SCALE;     // Kirby AABB in world units
    inline constexpr int ENEMY_COLL_PX = O_ENEMY_COLL_PX * SCALE;       // default enemy AABB in world units
}
