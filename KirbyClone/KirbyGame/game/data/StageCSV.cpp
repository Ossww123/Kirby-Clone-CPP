//
// Responsibility: Implement CSV readers for stage-related data tables.
// Non-Goals:      Robust CSV (quotes/escapes), detailed error reporting.
// Call-Context:   Main thread.
//
#include "game/data/StageCSV.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>  // strtol/strtof

namespace {

    // Strip UTF-8 BOM if present
    inline void strip_bom ( std::string& s ) {
        if ( s.size ( ) >= 3
            && static_cast< unsigned char >( s[ 0 ] ) == 0xEF
            && static_cast< unsigned char >( s[ 1 ] ) == 0xBB
            && static_cast< unsigned char >( s[ 2 ] ) == 0xBF ) {
            s.erase ( 0 , 3 );
        }
    }

    inline std::string trim ( std::string s ) {
        auto issp = [ ] ( unsigned char c ) { return std::isspace ( c ); };
        s.erase ( s.begin ( ) , std::find_if ( s.begin ( ) , s.end ( ) , [ & ] ( unsigned char c ) { return !issp ( c ); } ) );
        s.erase ( std::find_if ( s.rbegin ( ) , s.rend ( ) , [ & ] ( unsigned char c ) { return !issp ( c ); } ).base ( ) , s.end ( ) );
        return s;
    }

    // Very simple CSV: comma-split, no quotes
    inline std::vector<std::string> splitCSV ( const std::string& line ) {
        std::vector<std::string> out;
        std::stringstream ss ( line );
        std::string cell;
        while ( std::getline ( ss , cell , ',' ) ) out.push_back ( trim ( cell ) );
        return out;
    }

    inline char tolower_safe ( char c ) { return static_cast< char >( std::tolower ( static_cast< unsigned char >( c ) ) ); }

    inline int findIdx ( const std::vector<std::string>& hdr , const char* name ) {
        std::string key = name;
        std::transform ( key.begin ( ) , key.end ( ) , key.begin ( ) , tolower_safe );
        for ( int i = 0; i < static_cast< int >( hdr.size ( ) ); ++i ) {
            std::string n = hdr[ i ];
            std::transform ( n.begin ( ) , n.end ( ) , n.begin ( ) , tolower_safe );
            if ( n == key ) return i;
        }
        return -1;
    }

    // ---- C-locale numeric parsers (no exceptions) ----
    template<typename T> inline T to ( const std::string& s , T def );

    template<> inline int to<int> ( const std::string& s , int def ) {
        if ( s.empty ( ) ) return def;
        char* end = nullptr;
        long v = std::strtol ( s.c_str ( ) , &end , 10 );
        return ( end == s.c_str ( ) ) ? def : static_cast< int >( v );
    }

    template<> inline float to<float> ( const std::string& s , float def ) {
        if ( s.empty ( ) ) return def;
        char* end = nullptr;
        float v = std::strtof ( s.c_str ( ) , &end );
        return ( end == s.c_str ( ) ) ? def : v;
    }

    inline int to_bool01 ( const std::string& s , int def ) {
        if ( s.empty ( ) ) return def;
        std::string t = s; for ( auto& c : t ) c = tolower_safe ( c );
        if ( t == "1" || t == "true" || t == "yes" || t == "y" ) return 1;
        if ( t == "0" || t == "false" || t == "no" || t == "n" ) return 0;
        return def;
    }

    // Read first "valid header" line (skip comments/empty/BOM)
    inline bool read_header ( std::ifstream& f , std::vector<std::string>& outHdr ) {
        std::string line;
        while ( std::getline ( f , line ) ) {
            line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
            strip_bom ( line );
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;
            outHdr = splitCSV ( line );
            return !outHdr.empty ( );
        }
        return false;
    }

} // anon namespace

namespace game {

    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out ) {
        std::ifstream f ( path ); if ( !f ) return false;

        std::vector<std::string> hdr;
        if ( !read_header ( f , hdr ) ) return false;

        const int ix = findIdx ( hdr , "x" );
        const int iy = findIdx ( hdr , "y" );
        const int id = findIdx ( hdr , "dir" );
        if ( ix < 0 || iy < 0 || id < 0 ) return false;

        std::string line;
        if ( !std::getline ( f , line ) ) return false;
        line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
        line = trim ( line );

        const auto row = splitCSV ( line );
        if ( static_cast< int >( row.size ( ) ) <= std::max ( { ix, iy, id } ) ) return false;

        out.x = to<float> ( row[ ix ] , out.x );
        out.y = to<float> ( row[ iy ] , out.y );
        out.dir = to<int> ( row[ id ] , out.dir );
        return true;
    }

    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out ) {
        std::ifstream f ( path ); if ( !f ) return false;

        std::vector<std::string> hdr;
        if ( !read_header ( f , hdr ) ) return false;

        // Schema: type,x,y,dir,attack,move
        const int it = findIdx ( hdr , "type" );
        const int ix = findIdx ( hdr , "x" );
        const int iy = findIdx ( hdr , "y" );
        const int id = findIdx ( hdr , "dir" );
        const int ia = findIdx ( hdr , "attack" ); // optional
        const int im = findIdx ( hdr , "move" );   // optional

        if ( it < 0 || ix < 0 || iy < 0 || id < 0 ) return false;

        std::string line;
        out.reserve ( out.size ( ) + 64 );
        while ( std::getline ( f , line ) ) {
            line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;

            const auto row = splitCSV ( line );
            if ( static_cast< int >( row.size ( ) ) <= std::max ( { it, ix, iy, id } ) ) continue;

            MonsterCSV m{};
            m.type = row[ it ];
            m.x = to<float> ( row[ ix ] , 0.f );
            m.y = to<float> ( row[ iy ] , 0.f );
            m.dir = to<int> ( row[ id ] , 1 );

            if ( ia >= 0 && ia < static_cast< int >( row.size ( ) ) ) m.attack = to_bool01 ( row[ ia ] , m.attack );
            if ( im >= 0 && im < static_cast< int >( row.size ( ) ) ) m.move = to_bool01 ( row[ im ] , m.move );

            out.push_back ( std::move ( m ) );
        }
        return true;
    }

    bool LoadTileDefsCSV ( const char* path , std::vector<TileDefCSV>& out ) {
        std::ifstream f ( path ); if ( !f ) return false;

        std::vector<std::string> hdr;
        if ( !read_header ( f , hdr ) ) return false;

        const int iId = findIdx ( hdr , "id" );
        const int iS = findIdx ( hdr , "solid" );
        const int iO = findIdx ( hdr , "oneway" );
        const int iGx = findIdx ( hdr , "gx" );
        const int iGy = findIdx ( hdr , "gy" );

        if ( iId < 0 || ( iS < 0 && iO < 0 ) ) return false;

        std::string line;
        out.reserve ( out.size ( ) + 128 );
        while ( std::getline ( f , line ) ) {
            line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;

            const auto row = splitCSV ( line );
            TileDefCSV r{};
            if ( iId < static_cast< int >( row.size ( ) ) ) r.id = to<int> ( row[ iId ] , 0 );
            if ( iS < static_cast< int >( row.size ( ) ) ) r.solid = to<int> ( row[ iS ] , 0 );
            if ( iO < static_cast< int >( row.size ( ) ) ) r.oneway = to<int> ( row[ iO ] , 0 );
            if ( iGx >= 0 && iGx < static_cast< int >( row.size ( ) ) ) r.gx = to<int> ( row[ iGx ] , -1 );
            if ( iGy >= 0 && iGy < static_cast< int >( row.size ( ) ) ) r.gy = to<int> ( row[ iGy ] , -1 );

            out.push_back ( r );
        }
        return true;
    }

    bool LoadDoorsCSV ( const char* path , std::vector<DoorCSV>& out ) {
        out.clear ( );
        std::ifstream f ( path );
        if ( !f ) return false;

        // Use the same header reader for consistency
        std::vector<std::string> hdr;
        if ( !read_header ( f , hdr ) ) return false;

        const int ix = findIdx ( hdr , "x" );
        const int iy = findIdx ( hdr , "y" );
        const int iw = findIdx ( hdr , "w" );
        const int ih = findIdx ( hdr , "h" );
        const int it = findIdx ( hdr , "target" );
        if ( ix < 0 || iy < 0 || iw < 0 || ih < 0 || it < 0 ) return false;

        std::string line;
        while ( std::getline ( f , line ) ) {
            line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;

            const auto row = splitCSV ( line );
            if ( static_cast< int >( row.size ( ) ) <= std::max ( { ix, iy, iw, ih, it } ) ) continue;

            DoorCSV d{};
            d.x = to<int> ( row[ ix ] , 0 );
            d.y = to<int> ( row[ iy ] , 0 );
            d.w = to<int> ( row[ iw ] , 0 );
            d.h = to<int> ( row[ ih ] , 0 );
            d.target = row[ it ];
            out.push_back ( d );
        }
        return true;
    }

    // Numeric grid (tilemap.csv) loader
    bool LoadTileMapCSV ( const char* path , int& outW , int& outH , std::vector<int>& outIds ) {
        std::ifstream f ( path ); if ( !f ) return false;

        outIds.clear ( ); outW = -1; outH = 0;

        std::string line;
        while ( std::getline ( f , line ) ) {
            line.erase ( std::remove ( line.begin ( ) , line.end ( ) , '\r' ) , line.end ( ) );
            strip_bom ( line );
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;

            std::stringstream ss ( line );
            std::string cell;
            int cols = 0;

            while ( std::getline ( ss , cell , ',' ) ) {
                cell = trim ( cell );
                if ( cell.empty ( ) ) continue;
                outIds.push_back ( to<int> ( cell , 0 ) );
                ++cols;
            }

            if ( cols == 0 ) continue;
            if ( outW < 0 ) outW = cols;
            else if ( outW != cols ) return false; // inconsistent row width
            ++outH;
        }

        return ( outW > 0 && outH > 0 && static_cast< int >( outIds.size ( ) ) == outW * outH );
    }

} // namespace game
