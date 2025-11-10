#pragma once
//
// Responsibility: Orchestrate TileSet + TileMap + Collision as a single "world" unit.
//                 Supports an optional cover TileMap layer for hub blockers.
// Non-Goals:      File/CSV parsing, object rendering, editor policies.
// Call-Context:   Main thread. Render* must be called between SpriteBatch.Begin/End.
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
        // Tileset / Map (base)
        bool LoadTileset ( ID3D11Device* dev , const std::wstring& tilesPng , int cellW , int cellH );
        void SetWorldTileSize ( int tileW , int tileH ) { m_tiles.SetWorldTileSize ( tileW , tileH ); }
        bool SetMapFromMemory ( int w , int h , const int* ids ) { return m_map.LoadFromMemory ( w , h , ids ); }
        bool Load ( ID3D11Device* dev ,
                  const std::wstring& tilesPng ,
                  int cellW , int cellH ,      // atlas cell size (source)
                  int tileW , int tileH ,      // world tile size (dest)
                  int mapW , int mapH , const int* ids ); // map grid

        // Tile defs / Collision
        void DefineTile ( int id , const TileDef& def ) { m_tiles.Define ( id , def ); }
        void RebuildColliders ( ); // base + cover

        // Rendering
        void RenderVisible ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH ) const;
        void RenderVisibleScaled ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH , int scale ) const;

        // World extents (pixels)
        [[nodiscard]] IntRect WorldRectPx ( ) const; // {0,0, mapW*tileW, mapH*tileH}

        // Accessors
        const TileSet& Tiles ( )                      const { return m_tiles; }
        TileSet& Tiles ( )                                  { return m_tiles; }
        const TileMap& Map ( )                        const { return m_map; }
        physics::CollisionSystem& Collision ( )             { return m_collision; }
        const physics::CollisionSystem& Collision ( ) const { return m_collision; }

        int TileW ( ) const { return m_tiles.TileW ( ); }
        int TileH ( ) const { return m_tiles.TileH ( ); }
        int MapW ( )  const { return m_map.W ( ); }
        int MapH ( )  const { return m_map.H ( ); }

        // ---- Cover layer (hub blockers) ----
        bool SetCoverFromMemory ( int w , int h , const int* ids ); // enable & load
        void ClearCover ( );                                        // disable/clear
        void EraseCoverRectTiles ( int tx , int ty , int w , int h ); // fill -1 in rect

        bool HasCover ( ) const { return m_hasCover; }

    private:
        TileSet                  m_tiles{};
        TileMap                  m_map{};
        TileMap                  m_cover{};    // cover layer
        bool                     m_hasCover{ false };
        physics::CollisionSystem m_collision{};
    };

} // namespace engine
