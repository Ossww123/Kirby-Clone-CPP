#include "StageCSV.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {
    inline std::string trim ( std::string s ) {
        auto issp = [ ] ( unsigned char c ) { return std::isspace ( c ); };
        s.erase ( s.begin ( ) , std::find_if ( s.begin ( ) , s.end ( ) , [ & ] ( unsigned char c ) { return !issp ( c ); } ) );
        s.erase ( std::find_if ( s.rbegin ( ) , s.rend ( ) , [ & ] ( unsigned char c ) { return !issp ( c ); } ).base ( ) , s.end ( ) );
        return s;
    }
    // 아주 단순 CSV: 따옴표 미지원(필요없다면 충분)
    inline std::vector<std::string> splitCSV ( const std::string& line ) {
        std::vector<std::string> out;
        std::stringstream ss ( line );
        std::string cell;
        while ( std::getline ( ss , cell , ',' ) ) out.push_back ( trim ( cell ) );
        return out;
    }
    inline int findIdx ( const std::vector<std::string>& hdr , const char* name ) {
        for ( int i = 0; i < ( int ) hdr.size ( ); ++i ) {
            std::string n = hdr[ i ]; std::transform ( n.begin ( ) , n.end ( ) , n.begin ( ) , ::tolower );
            std::string k = name; std::transform ( k.begin ( ) , k.end ( ) , k.begin ( ) , ::tolower );
            if ( n == k ) return i;
        }
        return -1;
    }
    template<typename T> inline T to ( const std::string& s , T def );
    template<> inline int to<int> ( const std::string& s , int def ) { try { return s.empty ( ) ? def : std::stoi ( s ); } catch ( ... ) { return def; } }
    template<> inline float to<float> ( const std::string& s , float def ) { try { return s.empty ( ) ? def : std::stof ( s ); } catch ( ... ) { return def; } }
    inline int to_bool01 ( const std::string & s , int def ) {
        if ( s.empty ( ) ) return def;
        std::string t = s; for ( auto& c : t ) c = ( char ) tolower ( c );
        if ( t == "1" || t == "true" || t == "yes" || t == "y" ) return 1;
        if ( t == "0" || t == "false" || t == "no" || t == "n" ) return 0;
        return def;
    }
}

namespace game {

    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out ) {
        std::ifstream f ( path ); if ( !f ) return false;
        std::string line; if ( !std::getline ( f , line ) ) return false;
        auto hdr = splitCSV ( line );
        int ix = findIdx ( hdr , "x" ) , iy = findIdx ( hdr , "y" ) , id = findIdx ( hdr , "dir" );
        if ( ix < 0 || iy < 0 || id < 0 ) return false;
        if ( !std::getline ( f , line ) ) return false;
        auto row = splitCSV ( line );
        if ( ( int ) row.size ( ) <= std::max ( { ix,iy,id } ) ) return false;
        out.x = to<float> ( row[ ix ] , out.x );
        out.y = to<float> ( row[ iy ] , out.y );
        out.dir = to<int> ( row[ id ] , out.dir );
        return true;
    }

    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out ) {
        std::ifstream f ( path ); if ( !f ) return false;
        std::string line; if ( !std::getline ( f , line ) ) return false;
        auto hdr = splitCSV ( line );
        int it = findIdx ( hdr , "type" ) , ix = findIdx ( hdr , "x" ) , iy = findIdx ( hdr , "y" ) , id = findIdx ( hdr , "dir" );
        int iToX = findIdx ( hdr , "turnonhitx" ) , iTaE = findIdx ( hdr , "turnatedge" );
        int iWr = findIdx ( hdr , "wakerange" ) , iWu = findIdx ( hdr , "windupms" ) ,
            iFp = findIdx ( hdr , "fireperiod" ) , iBs = findIdx ( hdr , "bulletspeed" ) ,
            iSD = findIdx ( hdr , "stopduringwindup" );
        if ( it < 0 || ix < 0 || iy < 0 || id < 0 ) return false;
        while ( std::getline ( f , line ) ) {
            line = trim ( line );
            if ( line.empty ( ) || line[ 0 ] == '#' || line[ 0 ] == ';' ) continue;
            auto row = splitCSV ( line );
            if ( ( int ) row.size ( ) <= std::max ( { it,ix,iy,id } ) ) continue;
            MonsterCSV m;
            m.type = row[ it ];
            m.x = to<float> ( row[ ix ] , 0.f );
            m.y = to<float> ( row[ iy ] , 0.f );
            m.dir = to<int> ( row[ id ] , 1 );
            if ( iToX >= 0 && iToX < ( int ) row.size ( ) ) m.turnOnHitX = to_bool01 ( row[ iToX ] , -1 );
            if ( iTaE >= 0 && iTaE < ( int ) row.size ( ) ) m.turnAtEdge = to_bool01 ( row[ iTaE ] , -1 );
            
            if ( iWr >= 0 && iWr < ( int ) row.size ( ) ) m.wakeRange = to<float> ( row[ iWr ] , -1.f );
            if ( iWu >= 0 && iWu < ( int ) row.size ( ) ) m.windupMs = to<float> ( row[ iWu ] , -1.f );
            if ( iFp >= 0 && iFp < ( int ) row.size ( ) ) m.firePeriod = to<float> ( row[ iFp ] , -1.f );
            if ( iBs >= 0 && iBs < ( int ) row.size ( ) ) m.bulletSpeed = to<float> ( row[ iBs ] , -1.f );
            if ( iSD >= 0 && iSD < ( int ) row.size ( ) ) m.stopDuringWindup = to_bool01 ( row[ iSD ] , -1 );
            out.push_back ( std::move ( m ) );
        }
        return true;
    }

} // namespace game
