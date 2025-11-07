//
// Responsibility: CSV reader for animation clips ("strip"/"frame") and registration.
// Non-Goals:      Robust CSV (quotes/escapes), diagnostics beyond simple failure.
// Call-Context:   Main thread.
//
#include "game/data/AnimCSV.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <cstdlib>                  // strtol/strtof

#include "engine/util/Types.h"      // IntRect
#include "engine/util/Anim.h"       // engine::Animator, engine::AnimClip

namespace {

    // trim ASCII whitespace at both ends
    inline std::string trim ( const std::string& s ) {
        size_t a = 0 , b = s.size ( );
        while ( a < b && std::isspace ( ( unsigned char ) s[ a ] ) ) ++a;
        while ( b > a && std::isspace ( ( unsigned char ) s[ b - 1 ] ) ) --b;
        return s.substr ( a , b - a );
    }

    // very simple CSV split (no quotes)
    inline std::vector<std::string> splitCSV ( const std::string& line ) {
        std::vector<std::string> out;
        std::string cur;
        std::istringstream ss ( line );
        while ( std::getline ( ss , cur , ',' ) ) out.push_back ( trim ( cur ) );
        return out;
    }

    inline int   to_i ( const std::string& s ) { return static_cast< int >( std::strtol ( s.c_str ( ) , nullptr , 10 ) ); }
    inline float to_f ( const std::string& s ) { return std::strtof ( s.c_str ( ) , nullptr ); }
    inline bool  to_b ( const std::string& s ) {
        return ( s == "1" || s == "true" || s == "TRUE" || s == "True" );
    }

} // namespace

namespace game {

    bool LoadAnimCSV ( const char* filename , engine::Animator* anim , bool clearExisting ) {
        if ( !anim || !filename ) return false;

        std::ifstream in ( filename );
        if ( !in.is_open ( ) ) return false;

        if ( clearExisting ) anim->Clear ( );

        // accumulate frames per name, then register
        std::unordered_map<std::string , engine::AnimClip> clips;

        std::string line;
        int lineno = 0;
        while ( std::getline ( in , line ) ) {
            ++lineno;
            const std::string s = trim ( line );
            if ( s.empty ( ) || s[ 0 ] == '#' ) continue;

            const auto toks = splitCSV ( s );
            if ( toks.empty ( ) ) continue;

            const std::string& kind = toks[ 0 ];
            if ( kind == "strip" ) {
                // strip, name, sx,sy, fw,fh, count, dur, loop
                if ( toks.size ( ) < 9 ) continue;
                const std::string& name = toks[ 1 ];
                const int   sx = to_i ( toks[ 2 ] ) , sy = to_i ( toks[ 3 ] );
                const int   fw = to_i ( toks[ 4 ] ) , fh = to_i ( toks[ 5 ] );
                const int   count = to_i ( toks[ 6 ] );
                const float dur = to_f ( toks[ 7 ] );
                const bool  loop = to_b ( toks[ 8 ] );

                auto& clip = clips[ name ];
                clip.loop = loop; // last-seen loop wins
                for ( int i = 0; i < count; ++i ) {
                    engine::IntRect r{ sx + i * fw, sy, sx + ( i + 1 ) * fw, sy + fh };
                    clip.frames.push_back ( { r, dur } );
                }
            }
            else if ( kind == "frame" ) {
                // frame, name, sx,sy, w,h, dur, loop
                if ( toks.size ( ) < 8 ) continue;
                const std::string& name = toks[ 1 ];
                const int   sx = to_i ( toks[ 2 ] ) , sy = to_i ( toks[ 3 ] );
                const int   w = to_i ( toks[ 4 ] ) , h = to_i ( toks[ 5 ] );
                const float dur = to_f ( toks[ 6 ] );
                const bool  loop = to_b ( toks[ 7 ] );

                auto& clip = clips[ name ];
                clip.loop = loop;
                engine::IntRect r{ sx, sy, sx + w, sy + h };
                clip.frames.push_back ( { r, dur } );
            }
            else {
                // unknown kind: ignore
                continue;
            }
        }

        // register into Animator
        for ( auto& kv : clips ) {
            anim->AddClip ( kv.first.c_str ( ) , kv.second );
        }
        return true;
    }

} // namespace game
