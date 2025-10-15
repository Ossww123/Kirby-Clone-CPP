#include "engine/TileMap.h"
#include "engine/D3D11SpriteBatch.h" // Render에서 batch.Draw 사용

#include <fstream>
#include <sstream>
#include <algorithm>

namespace engine {

    bool TileMap::LoadCSV ( const wchar_t* path )
    {
        std::wifstream fin ( path );
        if ( !fin ) return false;

        std::vector<std::vector<int>> rows;
        std::wstring line;
        while ( std::getline ( fin , line ) ) {
            if ( line.empty ( ) ) continue;

            // 주석/공백 라인 스킵 (# 또는 ;)
            std::wstring trimmed = line;
            auto notsp = [ ] ( wchar_t c ) { return c != L' ' && c != L'\t' && c != L'\r'; };
            auto it = std::find_if ( trimmed.begin ( ) , trimmed.end ( ) , notsp );
            if ( it == trimmed.end ( ) ) continue;
            if ( *it == L'#' || *it == L';' ) continue;

            std::wstringstream ss ( line );
            std::wstring cell;
            std::vector<int> row;

            while ( std::getline ( ss , cell , L',' ) ) {
                if ( cell.empty ( ) ) { row.push_back ( -1 ); continue; }
                // 공백 제거
                size_t b = cell.find_first_not_of ( L" \t\r" );
                size_t e = cell.find_last_not_of ( L" \t\r" );
                if ( b == std::wstring::npos ) { row.push_back ( -1 ); continue; }
                std::wstring trimmed = cell.substr ( b , e - b + 1 );

                try {
                    row.push_back ( std::stoi ( trimmed ) );
                }
                catch ( ... ) {
                    row.push_back ( -1 );
                }
            }
            if ( !row.empty ( ) ) rows.push_back ( std::move ( row ) );
        }

        if ( rows.empty ( ) ) return false;

        m_h = ( int ) rows.size ( );
        m_w = ( int ) rows[ 0 ].size ( );
        for ( const auto& r : rows ) if ( ( int ) r.size ( ) != m_w ) return false; // 폭 불일치 방지

        m_ids.assign ( m_w * m_h , -1 );
        for ( int y = 0; y < m_h; ++y )
            for ( int x = 0; x < m_w; ++x )
                m_ids[ y * m_w + x ] = rows[ y ][ x ];

        return true;
    }

    void TileMap::BuildSolidColliders ( physics::CollisionSystem& cs , const TileSet& tiles ) const
    {
        struct R { int x , y , w , h; };

        std::vector<R> prev , next , out;
        prev.reserve ( 256 ); next.reserve ( 256 ); out.reserve ( 256 );

        auto isSolid = [ & ] ( int x , int y )->bool {
            if ( x < 0 || y < 0 || x >= m_w || y >= m_h ) return false;
            int id = At ( x , y );
            if ( const auto* def = tiles.Get ( id ) ) return def->solid;
            return false;
        };

        for ( int y = 0; y < m_h; ++y ) {
            std::vector<R> cur; cur.reserve ( 128 );
            int runStart = -1;

            // 수평 병합
            for ( int x = 0; x <= m_w; ++x ) {
                bool solid = ( x < m_w ) ? isSolid ( x , y ) : false;
                if ( solid ) {
                    if ( runStart < 0 ) runStart = x; 
                }
                else if ( runStart >= 0 ) {
                    cur.push_back ( { runStart, y, x - runStart, 1 } ); 
                    runStart = -1; 
                }
            }

            next.clear ( );
            std::vector<char> used ( cur.size ( ) , 0 );

            // 수직 병합
            for ( const auto& a : prev ) {
                bool merged = false;
                for ( size_t j = 0; j < cur.size ( ); ++j ) {
                    if ( used[ j ] ) continue;
                    const auto& b = cur[ j ];
                    if ( a.x == b.x && a.w == b.w ) {
                        R ext = a;
                        ext.h = a.h + 1;
                        next.push_back ( ext );
                        used[ j ] = 1;
                        merged = true;
                        break;
                    }
                }
                if ( !merged ) out.push_back ( a );  // 병합 불가 → 완성
            }

            for ( size_t j = 0; j < cur.size ( ); ++j ) {
                if ( !used[ j ] ) 
                    next.push_back ( cur[ j ] );
            }
            prev.swap ( next );
        }
        out.insert ( out.end ( ) , prev.begin ( ) , prev.end ( ) );

        const int tw = tiles.TileW ( ) , th = tiles.TileH ( );
        for ( const auto& r : out ) cs.AddStaticBox ( r.x * tw , r.y * th , r.w * tw , r.h * th );

        // --- ONEWAY: 타일 단위로 그대로 추가 ---
        for ( int y = 0; y < m_h; ++y ) {
            for ( int x = 0; x < m_w; ++x ) {
                int id = At ( x , y );
                const TileDef* def = tiles.Get ( id );
                if ( !def || !def->oneway ) continue;
                cs.AddOneWayBox ( x * tw , y * th , tw , th );
            }
        }
    }


    void TileMap::Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                         int camOffX , int camOffY , int screenW , int screenH ) const
    {
        if ( !tiles.Atlas ( ).srv ) return;
        const int tw = tiles.TileW ( ) , th = tiles.TileH ( );
        if ( tw <= 0 || th <= 0 || m_w <= 0 || m_h <= 0 ) return;

        // 화면 가시 타일 범위
        int x0 = std::max ( 0 , ( camOffX ) / tw );
        int y0 = std::max ( 0 , ( camOffY ) / th );
        int x1 = std::min ( m_w - 1 , ( camOffX + screenW ) / tw + 1 );
        int y1 = std::min ( m_h - 1 , ( camOffY + screenH ) / th + 1 );

        for ( int y = y0; y <= y1; ++y ) {
            for ( int x = x0; x <= x1; ++x ) {
                int id = At ( x , y );
                const TileDef* def = tiles.Get ( id );
                if ( !def ) continue;

                float px = float ( x * tw - camOffX );
                float py = float ( y * th - camOffY );
                batch.Draw ( tiles.Atlas ( ) , px , py , ( float ) tw , ( float ) th , &def->src , 0xFFFFFFFF );
            }
        }
    }

} // namespace engine
