#include "engine/TileMap.h"
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

        // ---- SOLID: 세로 방향 그리디 병합 ----
        for ( int x = 0; x < m_w; ++x ) {
            int y = 0;
            while ( y < m_h ) {
                int y0 = y;
                // 연속된 SOLID 런 탐색
                for ( ; y < m_h; ++y ) {
                    const int id = m_ids[ y * m_w + x ];
                    const TileDef* d = tiles.Get ( id );
                    if ( !( d && d->solid ) ) break;
                }
                const int y1 = y; // exclusive
                if ( y1 > y0 ) {
                    const float px = float ( x * tw );
                    const float py = float ( y0 * th );
                    const float pw = float ( tw );
                    const float ph = float ( ( y1 - y0 ) * th );
                    sys.AddStaticBox ( px , py , pw , ph );
                    ++added;
                }
                y = ( y1 == y0 ) ? ( y + 1 ) : y1;
            }
        }

        // ---- ONEWAY: 타일 단위(엔진 정책에 따라 높이 조정 가능) ----
        for ( int y = 0; y < m_h; ++y ) {
            for ( int x = 0; x < m_w; ++x ) {
                const int id = m_ids[ y * m_w + x ];
                const TileDef* d = tiles.Get ( id );
                if ( !( d && d->oneway ) ) continue;

                const float px = float ( x * tw );
                const float py = float ( y * th );
                const float pw = float ( tw );
                const float ph = float ( th ); // 필요 시 얇은 두께로 변경
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

    static inline bool hasRect ( const RECT& r ) {
        return ( r.right > r.left ) && ( r.bottom > r.top );
    }

    // src 선택: 1) 정의된 src 우선 2) id 기반 폴백(0/1-base) 3) 범위 밖이면 무시
    static inline bool pickSrcRect ( const TileSet& tiles , const TileDef* def , int id , RECT& out )
    {
        if ( def && !TileSet::IsEmptyRect ( def->src ) ) {
            out = def->src;
            return true;
        }
        // 0-based 인덱스 시도
        if ( tiles.TrySrcFromIndex ( id , &out ) ) return true;
        // 1-based 보정 시도
        if ( tiles.TrySrcFromIndex ( id - 1 , &out ) ) return true;
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

        const int tw = tiles.TileW ( );   // 월드 타일 크기 (예: 64)
        const int th = tiles.TileH ( );

        int tx0 , ty0 , tx1 , ty1;
        computeVisibleTileRect ( camOffX , camOffY , screenW , screenH , tw , th , m_w , m_h ,
                               tx0 , ty0 , tx1 , ty1 );

        for ( int y = ty0; y < ty1; ++y ) {
            for ( int x = tx0; x < tx1; ++x ) {
                const int id = m_ids[ y * m_w + x ];
                if ( id < 0 ) continue;

                const TileDef* def = tiles.Get ( id );
                RECT src{ 0,0,0,0 };
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

        const int tw = tiles.TileW ( );   // 월드 타일 크기 (예: 64)
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
                RECT src{ 0,0,0,0 };
                if ( !pickSrcRect ( tiles , def , id , src ) ) continue;

                const float px = float ( x * tw - camOffX ) * scale;
                const float py = float ( y * th - camOffY ) * scale;
                batch.Draw ( tiles.Atlas ( ) , px , py , dw , dh , &src , 0xFFFFFFFF );
            }
        }
    }

} // namespace engine
