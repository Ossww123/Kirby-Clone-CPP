#include "engine/world/TileSet.h"
#include "engine/render/TextureLoader.h" // LoadTextureWIC

namespace engine {

    bool TileSet::LoadAtlas ( ID3D11Device* dev , const wchar_t* path ,
                            int cellW , int cellH )
    {
        if ( !dev || !path || cellW <= 0 || cellH <= 0 ) return false;

        m_atlas = {};
        if ( !LoadTextureWIC ( dev , path , &m_atlas ) ) return false;

        m_cellW = cellW;
        m_cellH = cellH;

        // Default world tile size = cell size unless overridden
        if ( m_tileW <= 0 ) m_tileW = cellW;
        if ( m_tileH <= 0 ) m_tileH = cellH;

        return true;
    }

    bool TileSet::TrySrcFromIndex ( int idx , IntRect* out ) const
    {
        if ( !out || idx < 0 || !m_atlas.srv || m_cellW <= 0 || m_cellH <= 0 ) return false;

        const int c = Cols ( );
        if ( c <= 0 ) return false;

        const int gx = idx % c;
        const int gy = idx / c;

        const int left = gx * m_cellW;
        const int top = gy * m_cellH;
        const int right = left + m_cellW;
        const int bottom = top + m_cellH;

        if ( right > m_atlas.width || bottom > m_atlas.height ) return false;

        *out = IntRect{ left, top, right, bottom };
        return true;
    }

} // namespace engine
