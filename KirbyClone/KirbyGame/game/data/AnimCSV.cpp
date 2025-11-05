#include "game/AnimCSV.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>

namespace {
    inline std::string trim ( const std::string& s ) {
        size_t a = 0 , b = s.size ( );
        while ( a < b && std::isspace ( ( unsigned char ) s[ a ] ) ) ++a;
        while ( b > a && std::isspace ( ( unsigned char ) s[ b - 1 ] ) ) --b;
        return s.substr ( a , b - a );
    }
    inline std::vector<std::string> splitCSV ( const std::string& line ) {
        std::vector<std::string> out; std::string cur; std::istringstream ss ( line );
        // 아주 단순: 쉼표 기준, 따옴표 미지원(필요없을 것)
        while ( std::getline ( ss , cur , ',' ) ) out.push_back ( trim ( cur ) );
        return out;
    }
    inline int   to_i ( const std::string& s ) { return std::strtol ( s.c_str ( ) , nullptr , 10 ); }
    inline float to_f ( const std::string& s ) { return std::strtof ( s.c_str ( ) , nullptr ); }
    inline bool  to_b ( const std::string& s ) { return ( s == "1" || s == "true" || s == "TRUE" || s == "True" ); }
}

namespace game {

    bool LoadAnimCSV ( const char* filename , engine::Animator* anim , bool clearExisting )
    {
        if ( !anim || !filename ) return false;
        std::ifstream in ( filename );
        if ( !in.is_open ( ) ) return false;

        if ( clearExisting ) anim->Clear ( );

        // 이름별로 프레임을 누적한 후 마지막에 등록
        std::unordered_map<std::string , engine::AnimClip> clips;

        std::string line;
        int lineno = 0;
        while ( std::getline ( in , line ) ) {
            ++lineno;
            auto s = trim ( line );
            if ( s.empty ( ) || s[ 0 ] == '#' ) continue;

            auto t = splitCSV ( s );
            if ( t.empty ( ) ) continue;

            const std::string kind = t[ 0 ];
            if ( kind == "strip" ) {
                // strip, name, sx,sy, fw,fh, count, dur, loop
                if ( t.size ( ) < 9 ) continue;
                std::string name = t[ 1 ];
                int sx = to_i ( t[ 2 ] ) , sy = to_i ( t[ 3 ] );
                int fw = to_i ( t[ 4 ] ) , fh = to_i ( t[ 5 ] );
                int count = to_i ( t[ 6 ] );
                float dur = to_f ( t[ 7 ] );
                bool loop = to_b ( t[ 8 ] );

                auto& clip = clips[ name ];
                clip.loop = loop; // 마지막 strip/프레임의 loop 값이 우선
                for ( int i = 0; i < count; ++i ) {
                    RECT r{ sx + i * fw, sy, sx + ( i + 1 ) * fw, sy + fh };
                    clip.frames.push_back ( { r, dur } );
                }
            }
            else if ( kind == "frame" ) {
                // frame, name, sx,sy, w,h, dur, loop
                if ( t.size ( ) < 8 ) continue;
                std::string name = t[ 1 ];
                int sx = to_i ( t[ 2 ] ) , sy = to_i ( t[ 3 ] );
                int w = to_i ( t[ 4 ] ) , h = to_i ( t[ 5 ] );
                float dur = to_f ( t[ 6 ] );
                bool loop = to_b ( t[ 7 ] );

                auto& clip = clips[ name ];
                clip.loop = loop;
                RECT r{ sx, sy, sx + w, sy + h };
                clip.frames.push_back ( { r, dur } );
            }
            else {
                // 알 수 없는 kind 무시
                continue;
            }
        }

        // Animator에 등록
        for ( auto& kv : clips ) {
            anim->AddClip ( kv.first.c_str ( ) , kv.second );
        }
        return true;
    }

} // namespace game
