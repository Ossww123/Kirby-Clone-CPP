#pragma once
//
// Responsibility: Hold tile ID grid (W×H), accept grid from caller, and render visible tiles.
//                 - Computes visible window from camera offset + screen size
//                 - Submits draw calls using TileSet (src rect) + world tile size
//                 - Builds static colliders (from TileDef flags) on request
//
// Non-Goals:      - Any file/CSV parsing (handled by game::StageCSV)
//                 - Texture ownership (TileSet owns the atlas)
//                 - Advanced streaming/virtualization
//
// Call-Context:   Main thread. Render* must be called between SpriteBatch Begin/End.
//
// Coordinates:    - Map units are tiles. Destination size uses TileSet.TileW/TileH (world).
//                 - Source rects (atlas) use TileSet.CellW/CellH.
//                 - camOffX/camOffY are camera offsets in world pixels (top-left).
//

#include <vector>
#include <string>
#include "engine/TileSet.h"
#include "engine/D3D11SpriteBatch.h"
#include "engine/Collision.h"

namespace engine {

    class TileMap {
    public:
        // Inject grid from memory (row-major, size == w*h).
        bool LoadFromMemory ( int w , int h , const int* ids );

        // Build static colliders from current grid + TileSet defs.
        // - SOLID: vertical merge into tall boxes
        // - ONEWAY: one box per tile (engine-specific thickness policy)
        // Returns number of shapes added.
        int  BuildSolidColliders ( physics::CollisionSystem& sys , const TileSet& tiles ) const;

        // Render visible tiles (no scaling).
        void Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                      int camOffX , int camOffY , int screenW , int screenH ) const;

        // Render visible tiles with integer scaling.
        void RenderScaled ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                            int camOffX , int camOffY , int screenW , int screenH , int scale ) const;

        // Accessors
        int W ( ) const { return m_w; }
        int H ( ) const { return m_h; }
        int At ( int x , int y ) const { return m_ids[ y * m_w + x ]; }

    private:
        int m_w = 0 , m_h = 0;        // map width/height in tiles
        std::vector<int> m_ids;       // row-major tile IDs, size == m_w*m_h
    };

} // namespace engine
