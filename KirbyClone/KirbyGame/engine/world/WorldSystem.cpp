#include "engine/world/WorldSystem.h"

namespace engine {

    bool WorldSystem::LoadTileset ( ID3D11Device* dev , const std::wstring& tilesPng , int cellW , int cellH ) {
        return m_tiles.LoadAtlas ( dev , tilesPng.c_str ( ) , cellW , cellH );
    }

    bool WorldSystem::Load ( ID3D11Device* dev , const std::wstring& tilesPng ,
                             int cellW , int cellH , int tileW , int tileH ,
                             int mapW , int mapH , const int* ids )
    {
        if ( !LoadTileset ( dev , tilesPng , cellW , cellH ) ) return false;
        m_tiles.SetWorldTileSize ( tileW , tileH );
        if ( !m_map.LoadFromMemory ( mapW , mapH , ids ) )    return false;
        m_hasCover = false; // reset
        RebuildColliders ( );
        return true;
    }

    void WorldSystem::RebuildColliders ( ) {
        m_collision.Clear ( );
        m_map.BuildSolidColliders ( m_collision , m_tiles );
        if ( m_hasCover ) m_cover.BuildSolidColliders ( m_collision , m_tiles );
    }

    void WorldSystem::RenderVisible ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH ) const {
        m_map.Render ( batch , m_tiles , ox , oy , screenW , screenH );
        if ( m_hasCover ) m_cover.Render ( batch , m_tiles , ox , oy , screenW , screenH );
    }

    void WorldSystem::RenderVisibleScaled ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH , int scale ) const {
        m_map.RenderScaled ( batch , m_tiles , ox , oy , screenW , screenH , scale );
        if ( m_hasCover ) m_cover.RenderScaled ( batch , m_tiles , ox , oy , screenW , screenH , scale );
    }

    IntRect WorldSystem::WorldRectPx ( ) const {
        const int w = m_map.W ( ) * m_tiles.TileW ( );
        const int h = m_map.H ( ) * m_tiles.TileH ( );
        return IntRect{ 0,0,w,h };
    }

    // ---- cover APIs ----
    bool WorldSystem::SetCoverFromMemory ( int w , int h , const int* ids ) {
        if ( !m_cover.LoadFromMemory ( w , h , ids ) ) { m_hasCover = false; return false; }
        m_hasCover = true; return true;
    }

    void WorldSystem::ClearCover ( ) {
        m_cover = TileMap{}; m_hasCover = false;
    }

    void WorldSystem::EraseCoverRectTiles ( int tx , int ty , int w , int h ) {
        if ( !m_hasCover ) return;
        m_cover.FillRect ( tx , ty , w , h , -1 );
    }

} // namespace engine
