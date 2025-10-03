#include "engine/WorldSystem.h"

namespace engine {

    bool WorldSystem::Load ( ID3D11Device* dev ,
                           const std::wstring& tilesPng ,
                           const std::wstring& csv ,
                           int tileW , int tileH )
    {
        if ( !LoadTileset ( dev , tilesPng , tileW , tileH ) ) return false;
        if ( !LoadMapCSV ( csv ) ) return false;
        RebuildColliders ( );
        return true;
    }

    bool WorldSystem::LoadTileset ( ID3D11Device* dev , const std::wstring& tilesPng , int tileW , int tileH )
    {
        return m_tiles.LoadAtlas ( dev , tilesPng.c_str ( ) , tileW , tileH );
    }

    bool WorldSystem::LoadMapCSV ( const std::wstring& csv )
    {
        return m_map.LoadCSV ( csv.c_str ( ) );
    }

    void WorldSystem::RebuildColliders ( )
    {
        m_collision.Clear ( );
        m_map.BuildSolidColliders ( m_collision , m_tiles );
    }

    void WorldSystem::RenderVisible ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH ) const
    {
        m_map.Render ( batch , m_tiles , ox , oy , screenW , screenH );
    }

    RECT WorldSystem::WorldRectPx ( ) const
    {
        const int w = m_map.W ( ) * m_tiles.TileW ( );
        const int h = m_map.H ( ) * m_tiles.TileH ( );
        return RECT{ 0, 0, w, h };
    }

} // namespace engine
