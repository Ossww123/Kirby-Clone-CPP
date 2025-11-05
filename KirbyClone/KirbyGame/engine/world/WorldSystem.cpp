#include "engine/WorldSystem.h"

namespace engine {

    // ---- Tileset ----
    bool WorldSystem::LoadTileset ( ID3D11Device* dev ,
                                    const std::wstring& tilesPng ,
                                    int cellW , int cellH )
    {
        return m_tiles.LoadAtlas ( dev , tilesPng.c_str ( ) , cellW , cellH );
    }

    // ---- Optional one-shot loader (no CSV) ----
    bool WorldSystem::Load ( ID3D11Device* dev ,
                             const std::wstring& tilesPng ,
                             int cellW , int cellH ,
                             int tileW , int tileH ,
                             int mapW , int mapH , const int* ids )
    {
        if ( !LoadTileset ( dev , tilesPng , cellW , cellH ) ) return false;
        m_tiles.SetWorldTileSize ( tileW , tileH );
        if ( !m_map.LoadFromMemory ( mapW , mapH , ids ) )    return false;
        RebuildColliders ( );
        return true;
    }

    // ---- Collision ----
    void WorldSystem::RebuildColliders ( )
    {
        m_collision.Clear ( );
        m_map.BuildSolidColliders ( m_collision , m_tiles );
    }

    // ---- Rendering ----
    void WorldSystem::RenderVisible ( D3D11SpriteBatch& batch ,
                                      int ox , int oy , int screenW , int screenH ) const
    {
        m_map.Render ( batch , m_tiles , ox , oy , screenW , screenH );
    }

    void WorldSystem::RenderVisibleScaled ( D3D11SpriteBatch& batch ,
                                            int ox , int oy , int screenW , int screenH ,
                                            int scale ) const
    {
        m_map.RenderScaled ( batch , m_tiles , ox , oy , screenW , screenH , scale );
    }

    // ---- World rect ----
    RECT WorldSystem::WorldRectPx ( ) const
    {
        const int w = m_map.W ( ) * m_tiles.TileW ( );
        const int h = m_map.H ( ) * m_tiles.TileH ( );
        return RECT{ 0, 0, w, h };
    }

} // namespace engine
