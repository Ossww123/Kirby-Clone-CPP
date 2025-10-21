#pragma once
#include <vector>
#include <string>

#include "engine/TileSet.h"
#include "engine/Collision.h"   // physics::CollisionSystem

namespace engine {

    class D3D11SpriteBatch;

    // Lightweight, row-major tile map (IDs) with CSV loading and batched render.
    // World/physics units use the tileset's tileW/tileH (e.g., 16x16).
    // For pixel scaling (e.g., 4x upscaling on screen), use RenderScaled().
    class TileMap {
    public:
        // CSV format: each line is a row, comma-separated.
        // Empty cell / non-number → -1 (empty).
        bool LoadCSV ( const wchar_t* path );

        // Merge SOLID tiles into larger rectangles and register to the collision system.
        void BuildSolidColliders ( physics::CollisionSystem& cs , const TileSet& tiles ) const;

        // Render only the visible tiles under the given camera offset and screen size.
        // Draws at a 1:1 ratio (no on-screen scaling).
        void Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                    int camOffX , int camOffY , int screenW , int screenH ) const;

        // Same as Render(), but applies an integer on-screen scale factor.
        // World/physics remain unchanged; only draw positions/sizes are multiplied by `scale`.
        void RenderScaled ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                          int camOffX , int camOffY , int screenW , int screenH , int scale ) const;

        // Accessors
        int W ( ) const { return m_w; }
        int H ( ) const { return m_h; }
        int At ( int x , int y ) const { return m_ids[ y * m_w + x ]; }

    private:
        int m_w = 0 , m_h = 0;           // map width/height in tiles
        std::vector<int> m_ids;         // row-major tile IDs, size = m_w * m_h
    };

} // namespace engine
