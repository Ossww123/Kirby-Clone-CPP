#pragma once
//
// Responsibility: Runtime tile layer wrapper (TileSet + TileMap + world offset/z/collision).
// Non-Goals:      CSV parsing, high-level stage/scene logic, editor UI.
// Call-Context:   Main thread; used by PlaySession / stage loader.
//

#include <string>

#include "engine/world/TileMap.h"
#include "engine/world/TileSet.h"

namespace engine {
    class D3D11SpriteBatch;
}

namespace engine::physics {
    class CollisionSystem;
}

namespace game {

    struct TileLayerRuntime {
        // Authoring/debug name (e.g. "ground", "waterfall")
        std::string name;

        // World-space origin of this layer in pixels (top-left of tile (0,0)).
        int offsetX = 0;
        int offsetY = 0;

        // Draw order: smaller = background, larger = foreground.
        int  z = 0;

        // If true, this layer contributes solid/one-way collision.
        bool collides = true;

        // Data
        engine::TileSet tiles;  // atlas + per-tile defs
        engine::TileMap map;    // tile-id grid

        // Build collision shapes for this layer, if enabled.
        void BuildColliders ( engine::physics::CollisionSystem& sys ) const;

        // Per-frame update hook (reserved for animated tiles, etc.).
        void Tick ( float /*dt*/ ) { /* no-op for now */ }

        // Render with 1x destination scale.
        void Render ( engine::D3D11SpriteBatch& batch ,
                      int camX , int camY ,
                      int screenW , int screenH ) const;

        // Render with integer destination scale.
        void RenderScaled ( engine::D3D11SpriteBatch& batch ,
                            int camX , int camY ,
                            int screenW , int screenH ,
                            int scale ) const;
    };

} // namespace game
