//
// Responsibility: Flat JSON reader for StageDesc (string/number values + optional boss.arena).
// Non-Goals:      Robust JSON (quoting/escapes/nesting beyond minimal), detailed diagnostics.
// Call-Context:   Main thread.
//
#include "game/data/StageDesc.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <string>

namespace {
    // Read whole file into a string (binary to preserve bytes)
    bool read_file ( const char* path , std::string& out ) {
        std::ifstream ifs ( path , std::ios::in | std::ios::binary );
        if ( !ifs ) return false;
        std::ostringstream oss; oss << ifs.rdbuf ( );
        out = oss.str ( );
        return true;
    }
} // namespace

namespace game {

    bool LoadStageDesc ( const char* jsonPath , StageDesc& out ) {
        std::string s;
        if ( !jsonPath || !read_file ( jsonPath , s ) ) return false;

        // Flat pairs: "key":"value"  |  "key": number
        static const std::regex kv_re (
            R"REGEX("([A-Za-z0-9_]+)"\s*:\s*(?:"([^"]*)"|([-+]?[0-9]*\.?[0-9]+)))REGEX"
        );

        std::smatch m;
        auto it = s.cbegin ( );
        while ( std::regex_search ( it , s.cend ( ) , m , kv_re ) ) {
            const std::string key = m[ 1 ].str ( );
            const bool isString = m[ 2 ].matched;

            if ( key == "tileset" && isString ) out.tileset = m[ 2 ].str ( );
            else if ( key == "tiledefs" && isString ) out.tiledefs = m[ 2 ].str ( );
            else if ( key == "tilemap" && isString ) out.tilemap = m[ 2 ].str ( );
            else if ( key == "monsters" && isString ) out.monsters = m[ 2 ].str ( );
            else if ( key == "player_start" && isString ) out.player_start = m[ 2 ].str ( );
            else if ( key == "background" && isString ) out.background = m[ 2 ].str ( );
            else if ( key == "doors" && isString ) out.doors = m[ 2 ].str ( );

            it = m.suffix ( ).first;
        }

        // Optional nested: "boss": { ... "arena": { "x":..,"y":..,"w":..,"h":.. } }
        {
            static const std::regex boss_re (
                R"REGEX("boss"\s*:\s*\{[^}]*"arena"\s*:\s*\{([^}]*)\})REGEX" ,
                std::regex::icase
            );
            std::smatch bm;
            if ( std::regex_search ( s , bm , boss_re ) ) {
                const std::string inner = bm[ 1 ].str ( );
                auto find_int = [ & ] ( const char* k , int& dst )->bool {
                    std::regex r ( std::string ( "\"" ) + k + R"("\s*:\s*([-+]?[0-9]+))" );
                    std::smatch mm;
                    if ( std::regex_search ( inner , mm , r ) ) { dst = std::stoi ( mm[ 1 ].str ( ) ); return true; }
                    return false;
                    };
                int bx = 0 , by = 0 , bw = 0 , bh = 0;
                const bool ok = find_int ( "x" , bx ) && find_int ( "y" , by ) && find_int ( "w" , bw ) && find_int ( "h" , bh );
                if ( ok ) {
                    out.has_boss_arena = true;
                    out.boss_x = bx; out.boss_y = by; out.boss_w = bw; out.boss_h = bh;
                }
            }
        }

        // Required keys
        if ( out.tileset.empty ( ) || out.tiledefs.empty ( ) || out.tilemap.empty ( )
            || out.monsters.empty ( ) || out.player_start.empty ( ) || out.background.empty ( ) )
            return false;

        return true;
    }

} // namespace game
