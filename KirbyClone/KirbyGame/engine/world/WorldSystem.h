#pragma once
//
// Responsibility: Orchestrate TileSet + TileMap + Collision as a single "world" unit.
//                 - Own TileSet (atlas + cell/world sizes), TileMap (ID grid), CollisionSystem
//                 - Accept map grid from caller (memory), not from CSV
//                 - Rebuild static colliders from map + per-tile defs
//                 - Render only visible tiles (given camera offset / screen size)
//                 - Provide world size in pixels for camera clamping
//
// Non-Goals:      File/CSV parsing, object rendering, editor policies.
// Call-Context:   Main thread. Render* must be called between SpriteBatch.Begin/End.
//
// Coordinates:    Map units are tiles; each tile = (TileSet.TileW, TileSet.TileH) pixels.
//                 ox/oy are camera offsets in world pixels (screen top-left in world space).
//

#include <string>
#include "engine/world/TileSet.h"
#include "engine/world/TileMap.h"
#include "engine/physics/Collision.h"
#include "engine/util/Types.h" // IntRect

struct ID3D11Device;

namespace engine {
    class D3D11SpriteBatch; // fwd (render header not needed here)

    class WorldSystem {
    public:
        // ---- Tileset setup ----
        // Load tileset atlas and remember *cell* size (e.g., 16x16).
        bool LoadTileset ( ID3D11Device* dev , const std::wstring& tilesPng ,
                         int cellW , int cellH );

        // Set *world* tile size (e.g., 64x64).
        void SetWorldTileSize ( int tileW , int tileH ) { m_tiles.SetWorldTileSize ( tileW , tileH ); }

        // ---- Map setup (no CSV; memory injection only) ----
        bool SetMapFromMemory ( int w , int h , const int* ids ) { return m_map.LoadFromMemory ( w , h , ids ); }

        // ---- Optional convenience: do all at once (no CSV) ----
        bool Load ( ID3D11Device* dev ,
                  const std::wstring& tilesPng ,
                  int cellW , int cellH ,      // atlas cell size (source)
                  int tileW , int tileH ,      // world tile size (dest)
                  int mapW , int mapH , const int* ids ); // map grid

        // ---- Tile definitions ----
        // Typical flow: DefineTile(...) for all ids → RebuildColliders()
        void DefineTile ( int id , const TileDef& def ) { m_tiles.Define ( id , def ); }

        // ---- Collision ----
        void RebuildColliders ( ); // SOLID merge + ONEWAY per-tile

        // ---- Rendering ----
        void RenderVisible ( D3D11SpriteBatch& batch ,
                           int ox , int oy , int screenW , int screenH ) const;

        void RenderVisibleScaled ( D3D11SpriteBatch& batch ,
                                 int ox , int oy , int screenW , int screenH ,
                                 int scale ) const;

        // ---- World extents (pixels) ----
        [[nodiscard]] IntRect WorldRectPx ( ) const; // {0,0, mapW*tileW, mapH*tileH}

        // ---- Accessors ----
        const TileSet& Tiles ( ) const { return m_tiles; }
        TileSet& Tiles ( ) { return m_tiles; }
        const TileMap& Map ( )   const { return m_map; }
        physics::CollisionSystem& Collision ( ) { return m_collision; }
        const physics::CollisionSystem& Collision ( ) const { return m_collision; }

        int TileW ( ) const { return m_tiles.TileW ( ); }
        int TileH ( ) const { return m_tiles.TileH ( ); }
        int MapW ( )  const { return m_map.W ( ); }
        int MapH ( )  const { return m_map.H ( ); }

    private:
        TileSet                  m_tiles{};
        TileMap                  m_map{};
        physics::CollisionSystem m_collision{};
    };

} // namespace engine
