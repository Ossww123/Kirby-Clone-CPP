#pragma once
//
// Responsibility: Manage a tile atlas texture and per-tile definitions (logic + visual).
//                 - Owns atlas SRV and *cell* size (source slice size, e.g., 16x16)
//                 - Stores *world* tile size (destination size, e.g., 64x64)
//                 - Provides TileDef lookup and atlas-rect helpers
//
// Non-Goals:      - File/CSV parsing of any kind (handled by game::StageCSV)
//                 - Rendering submission (TileMap/WorldSystem drive SpriteBatch)
//                 - Physics building (TileMap builds from TileDef flags)
//
// Call-Context:   Main thread. After LoadAtlas(), read-only except Define()/SetWorldTileSize().
//
// Coordinates:    - RECT src is in *atlas pixel* coordinates (cellW/cellH grid).
//                 - World placement uses *tileW/tileH* (destination size).
//

#include <windows.h>
#include <unordered_map>
#include <string>
#include "engine/Texture.h" // Tex2D{ ID3D11ShaderResourceView* srv; int width,height; }

struct ID3D11Device;

namespace engine {

    struct TileDef {
        bool solid = false;            // blocks movement
        bool oneway = false;            // one-way platform
        RECT src{ 0,0,0,0 };        // atlas pixel rect (if empty -> auto fallback)
    };

    class TileSet {
    public:
        // Load atlas texture and remember *cell* size (e.g., 16x16).
        // Pre: dev!=nullptr, path!=nullptr, cellW>0, cellH>0
        bool LoadAtlas ( ID3D11Device* dev , const wchar_t* path ,
                         int cellW , int cellH );

        // Set *world* tile size (e.g., 64x64). Can be called anytime after LoadAtlas().
        void SetWorldTileSize ( int tileW , int tileH ) { m_tileW = tileW; m_tileH = tileH; }

        // Define / lookup per-tile metadata (logic + visual).
        void Define ( int id , const TileDef& def ) { m_defs[ id ] = def; }
        const TileDef* Get ( int id ) const {
            auto it = m_defs.find ( id );
            return ( it == m_defs.end ( ) ) ? nullptr : &it->second;
        }

        // Atlas helpers
        int  Cols ( ) const { return ( m_cellW > 0 ) ? ( m_atlas.width / m_cellW ) : 0; }
        int  Rows ( ) const { return ( m_cellH > 0 ) ? ( m_atlas.height / m_cellH ) : 0; }
        int  MaxIndex ( ) const { const int c = Cols ( ) , r = Rows ( ); return ( c > 0 && r > 0 ) ? ( c * r - 1 ) : -1; }
        static bool IsEmptyRect ( const RECT& r ) { return ( r.left >= r.right ) || ( r.top >= r.bottom ); }
        bool TrySrcFromIndex ( int idx , RECT* out ) const;

        // Accessors
        const Tex2D& Atlas ( ) const { return m_atlas; }
        int  CellW ( ) const { return m_cellW; }   // e.g., 16
        int  CellH ( ) const { return m_cellH; }   // e.g., 16
        int  TileW ( ) const { return m_tileW; }   // e.g., 64
        int  TileH ( ) const { return m_tileH; }   // e.g., 64

    private:
        Tex2D m_atlas{};                           // atlas SRV + size (pixels)
        int   m_cellW = 0 , m_cellH = 0;           // source cell size  (atlas slice)
        int   m_tileW = 0 , m_tileH = 0;           // world tile size   (destination)
        std::unordered_map<int , TileDef> m_defs;   // id -> definition
    };

} // namespace engine
