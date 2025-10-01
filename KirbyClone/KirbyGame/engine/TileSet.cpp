#include "engine/TileSet.h"
#include "engine/TextureLoader.h" // LoadTextureWIC

namespace engine {

    bool TileSet::LoadAtlas ( ID3D11Device* dev , const wchar_t* path , int tileW , int tileH )
    {
        if ( !dev || !path || tileW <= 0 || tileH <= 0 ) return false;

        m_atlas = {};
        if ( !LoadTextureWIC ( dev , path , &m_atlas ) ) return false;

        m_tileW = tileW;
        m_tileH = tileH;
        return true;
    }

} // namespace engine
