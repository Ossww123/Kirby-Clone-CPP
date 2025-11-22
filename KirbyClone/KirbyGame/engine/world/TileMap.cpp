#include "engine/world/TileMap.h"
#include "engine/world/TileSet.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/physics/Collision.h"
#include "engine/util/Types.h" // IntRect

#include <algorithm>

namespace engine {

    // -----------------------------
    // LoadFromMemory
    // -----------------------------
    bool TileMap::LoadFromMemory ( int w , int h , const int* ids )
    {
        if ( w <= 0 || h <= 0 || !ids ) return false;
        m_w = w; m_h = h;
        m_ids.assign ( ids , ids + ( w * h ) );
        return true;
    }

    // -----------------------------
    // Collision building
    // -----------------------------
    int TileMap::BuildSolidColliders ( physics::CollisionSystem& sys , const TileSet& tiles ) const
    {
        if ( m_w <= 0 || m_h <= 0 ) return 0;
        if ( tiles.TileW ( ) <= 0 || tiles.TileH ( ) <= 0 ) return 0;

        const int tw = tiles.TileW ( );
        const int th = tiles.TileH ( );
        int added = 0;

        // ---- SOLID: vertical greedy merge ----
        for ( int x = 0; x < m_w; ++x ) {
            int y = 0;
            while ( y < m_h ) {
                const int y0 = y;
                // find a solid run on column x
                for ( ; y < m_h; ++y ) {
                    const int id = m_ids[ y * m_w + x ];
                    const TileDef* d = tiles.Get ( id );
                    if ( !( d && d->solid ) ) break;
                }
                const int y1 = y; // exclusive
                if ( y1 > y0 ) {
                    const int px = x * tw;
                    const int py = y0 * th;
                    const int pw = tw;
                    const int ph = ( y1 - y0 ) * th;
                    sys.AddStaticBox ( px , py , pw , ph );
                    ++added;
                }
                y = ( y1 == y0 ) ? ( y + 1 ) : y1;
            }
        }

        // ---- ONEWAY: per-tile (height policy can be adjusted) ----
        for ( int y = 0; y < m_h; ++y ) {
            for ( int x = 0; x < m_w; ++x ) {
                const int id = m_ids[ y * m_w + x ];
                const TileDef* d = tiles.Get ( id );
                if ( !( d && d->oneway ) ) continue;

                const int px = x * tw;
                const int py = y * th;
                const int pw = tw;
                const int ph = th; // can be tightened by policy
                sys.AddOneWayBox ( px , py , pw , ph );
                ++added;
            }
        }

        return added;
    }

    // -----------------------------
    // Helpers
    // -----------------------------
    static inline void computeVisibleTileRect (
        int camOffX , int camOffY , int screenW , int screenH ,
        int tileW , int tileH , int mapW , int mapH ,
        int& tx0 , int& ty0 , int& tx1 , int& ty1 )
    {
        if ( tileW <= 0 || tileH <= 0 || mapW <= 0 || mapH <= 0 ) {
            tx0 = ty0 = 0; tx1 = ty1 = 0; return;
        }
        // world px -> tile index
        tx0 = std::max ( 0 , camOffX / tileW );
        ty0 = std::max ( 0 , camOffY / tileH );
        tx1 = std::min ( mapW , ( camOffX + screenW + tileW - 1 ) / tileW );
        ty1 = std::min ( mapH , ( camOffY + screenH + tileH - 1 ) / tileH );
    }

    // src pick order:
    // 1) explicit src in TileDef
    // 2) atlas index by id (0-based)
    // 3) atlas index by (id-1) as 1-based fallback
    static inline bool pickSrcRect ( const TileSet& tiles , const TileDef* def , int id , IntRect& out )
    {
        if ( def && !TileSet::IsEmpty ( def->src ) ) {
            out = def->src;
            return true;
        }
        if ( tiles.TrySrcFromIndex ( id , &out ) ) return true;       // 0-based
        if ( tiles.TrySrcFromIndex ( id - 1 , &out ) ) return true;   // 1-based fallback
        return false;
    }

    // -----------------------------
    // Render (1x)
    // -----------------------------
    void TileMap::Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                           int camOffX , int camOffY , int screenW , int screenH ) const
    {
        if ( !tiles.Atlas ( ).srv ) return;
        if ( m_w <= 0 || m_h <= 0 ) return;
        if ( tiles.TileW ( ) <= 0 || tiles.TileH ( ) <= 0 ) return;

        const int tw = tiles.TileW ( );
        const int th = tiles.TileH ( );

        int tx0 , ty0 , tx1 , ty1;
        computeVisibleTileRect ( camOffX , camOffY , screenW , screenH , tw , th , m_w , m_h ,
                                 tx0 , ty0 , tx1 , ty1 );

        for ( int y = ty0; y < ty1; ++y ) {
            for ( int x = tx0; x < tx1; ++x ) {
                const int id = m_ids[ y * m_w + x ];
                if ( id < 0 ) continue;

                const TileDef* def = tiles.Get ( id );
                IntRect src{ 0,0,0,0 };
                if ( !pickSrcRect ( tiles , def , id , src ) ) continue;

                const float px = float ( x * tw - camOffX );
                const float py = float ( y * th - camOffY );
                batch.Draw ( tiles.Atlas ( ) , px , py , float ( tw ) , float ( th ) , &src , 0xFFFFFFFF );
            }
        }
    }

    // -----------------------------
    // RenderScaled (dst integer scale)
    // -----------------------------
    void TileMap::RenderScaled ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                                 int camOffX , int camOffY , int screenW , int screenH , int scale ) const
    {
        if ( !tiles.Atlas ( ).srv ) return;
        if ( m_w <= 0 || m_h <= 0 ) return;
        if ( tiles.TileW ( ) <= 0 || tiles.TileH ( ) <= 0 ) return;
        if ( scale <= 0 ) return;

        const int tw = tiles.TileW ( );
        const int th = tiles.TileH ( );

        int tx0 , ty0 , tx1 , ty1;
        computeVisibleTileRect ( camOffX , camOffY , screenW , screenH , tw , th , m_w , m_h ,
                                 tx0 , ty0 , tx1 , ty1 );

        const float dw = float ( tw * scale );
        const float dh = float ( th * scale );

        for ( int y = ty0; y < ty1; ++y ) {
            for ( int x = tx0; x < tx1; ++x ) {
                const int id = m_ids[ y * m_w + x ];
                if ( id < 0 ) continue;

                const TileDef* def = tiles.Get ( id );
                IntRect src{ 0,0,0,0 };
                if ( !pickSrcRect ( tiles , def , id , src ) ) continue;

                const float px = float ( ( x * tw - camOffX ) * scale );
                const float py = float ( ( y * th - camOffY ) * scale );
                batch.Draw ( tiles.Atlas ( ) , px , py , dw , dh , &src , 0xFFFFFFFF );
            }
        }
    }

    bool TileMap::SetAt ( int x , int y , int id ) {
        if ( x < 0 || y < 0 || x >= m_w || y >= m_h ) return false;
        m_ids[ y * m_w + x ] = id;
        return true;
    }

    void TileMap::FillRect ( int tx , int ty , int w , int h , int id ) {
        if ( w <= 0 || h <= 0 ) return;
        const int x1 = std::min ( m_w , tx + w );
        const int y1 = std::min ( m_h , ty + h );
        for ( int y = std::max ( 0 , ty ); y < y1; ++y )
            for ( int x = std::max ( 0 , tx ); x < x1; ++x )
                m_ids[ y * m_w + x ] = id;
    }

} // namespace engine
