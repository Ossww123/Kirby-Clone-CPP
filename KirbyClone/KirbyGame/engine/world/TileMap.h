#pragma once
//
// Responsibility: Store tile-id grid, render visible tiles, and build static/one-way colliders.
// Non-Goals:      CSV parsing, camera logic, batching beyond sprite batch.
// Call-Context:   Main thread; integer world coordinates (pixels).
//

#include <vector>

namespace engine {
    class TileSet;
    class D3D11SpriteBatch;
}

namespace engine::physics {
    class CollisionSystem;
}

namespace engine {

    class TileMap {
    public:
        // Inject grid from memory (row-major, size == w*h).
        bool LoadFromMemory ( int w , int h , const int* ids );

        // Build static/one-way colliders from current grid + TileSet defs.
        // - SOLID: vertical merge into tall boxes
        // - ONEWAY: one box per tile (engine-specific thickness policy)
        // Returns number of shapes added.
        int BuildSolidColliders ( physics::CollisionSystem& sys , const TileSet& tiles ) const;

        // Render visible tiles (no additional scaling).
        void Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                    int camOffX , int camOffY , int screenW , int screenH ) const;

        // Render visible tiles with integer scaling.
        void RenderScaled ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                          int camOffX , int camOffY , int screenW , int screenH , int scale ) const;

        // Mutators (used by cover-layer editing)
        bool SetAt ( int x , int y , int id );
        void FillRect ( int tx , int ty , int w , int h , int id );

        // Accessors
        [[nodiscard]] int W ( ) const { return m_w; }
        [[nodiscard]] int H ( ) const { return m_h; }
        [[nodiscard]] int At ( int x , int y ) const { return m_ids[ y * m_w + x ]; }

    private:
        int m_w = 0 , m_h = 0;         // map width/height in tiles
        std::vector<int> m_ids;       // row-major tile IDs, size == m_w*m_h
    };

} // namespace engine
