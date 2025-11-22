#pragma once
//
// Responsibility: Tile atlas (D3D11 SRV + cell slicing) and per-tile metadata.
// Non-Goals:      CSV map loading, rendering, collision building.
// Call-Context:   Main thread; header-only helpers; no Win32 types.
//

#include <cstdint>
#include <unordered_map>
#include <string>

#include "engine/render/Texture.h" // Tex2D (srv/width/height)
#include "engine/util/Types.h"     // IntRect

struct ID3D11Device;

namespace engine {

    // Optional slope shape for ground tiles.
    enum class TileSlope : std::uint8_t {
        None = 0 ,
        UpRight45 ,   // 45deg,    low at left,  high at right
        UpLeft45 ,    // 45deg,    high at left, low at right
        UpRight22 ,   // ~22.5deg, low at left,  high at right
        UpLeft22      // ~22.5deg, high at left, low at right
    };

    struct TileDef {
        bool    solid      = false;            
        bool    oneway     = false;           
        bool    water      = false;            
        bool    ladder     = false;         
        bool    star_block = false;
        TileSlope slope = TileSlope::None;
        IntRect src{ 0, 0, 0, 0 };
    };

    class TileSet {
    public:
        // Load atlas texture and remember cell (source slice) size, e.g. 16x16.
        // Pre: dev!=nullptr, path!=nullptr, cellW>0, cellH>0
        bool LoadAtlas ( ID3D11Device* dev , const wchar_t* path ,
                         int cellW , int cellH );

        // Set world tile size (destination size), e.g. 64x64.
        void SetWorldTileSize ( int tileW , int tileH ) { m_tileW = tileW; m_tileH = tileH; }

        // Define / lookup per-tile metadata (logic + visual).
        void Define ( int id , const TileDef& def ) { m_defs[ id ] = def; }
        [[nodiscard]] const TileDef* Get ( int id ) const {
            auto it = m_defs.find ( id );
            return ( it == m_defs.end ( ) ) ? nullptr : &it->second;
        }

        // Atlas helpers (cell grid based on atlas pixels)
        [[nodiscard]] int Cols ( ) const { return ( m_cellW > 0 ) ? ( m_atlas.width / m_cellW ) : 0; }
        [[nodiscard]] int Rows ( ) const { return ( m_cellH > 0 ) ? ( m_atlas.height / m_cellH ) : 0; }
        [[nodiscard]] int MaxIndex ( ) const {
            const int c = Cols ( ) , r = Rows ( );
            return ( c > 0 && r > 0 ) ? ( c * r - 1 ) : -1;
        }
        [[nodiscard]] static bool IsEmpty ( const IntRect& r ) noexcept {
            return ( r.l >= r.r ) || ( r.t >= r.b );
        }

        // Compute atlas rect by linear index (0..MaxIndex). Returns false if out-of-range.
        bool TrySrcFromIndex ( int idx , IntRect* out ) const;

        // Accessors
        [[nodiscard]] const Tex2D& Atlas ( ) const { return m_atlas; }
        [[nodiscard]] int  CellW ( ) const { return m_cellW; }   // e.g., 16
        [[nodiscard]] int  CellH ( ) const { return m_cellH; }   // e.g., 16
        [[nodiscard]] int  TileW ( ) const { return m_tileW; }   // e.g., 64
        [[nodiscard]] int  TileH ( ) const { return m_tileH; }   // e.g., 64

    private:
        Tex2D m_atlas{};                           // atlas SRV + size (pixels)
        int   m_cellW = 0 , m_cellH = 0;           // source cell size (atlas slice)
        int   m_tileW = 0 , m_tileH = 0;           // world tile size (destination)
        std::unordered_map<int , TileDef> m_defs;  // id -> definition
    };

} // namespace engine
