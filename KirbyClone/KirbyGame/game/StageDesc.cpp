#include "StageDesc.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace {
    bool read_file ( const char* path , std::string& out ) {
        std::ifstream ifs ( path , std::ios::in | std::ios::binary );
        if ( !ifs ) return false;
        std::ostringstream oss; oss << ifs.rdbuf ( );
        out = oss.str ( ); return true;
    }
}

namespace game {
    bool LoadStageDesc ( const char* jsonPath , StageDesc& out ) {
        std::string s; if ( !read_file ( jsonPath , s ) ) return false;

        // "key": "value"  | "key": number
        static const std::regex kv_re (
            R"REGEX("([A-Za-z0-9_]+)"\s*:\s*(?:"([^"]*)"|([-+]?[0-9]*\.?[0-9]+)))REGEX"
        );

        std::smatch m; auto it = s.cbegin ( );
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

        // 필수 키 검증 (배경은 선택)
        if ( out.tileset.empty ( ) || out.tiledefs.empty ( ) || out.tilemap.empty ( )
            || out.monsters.empty ( ) || out.player_start.empty ( ) || out.background.empty ( ) )
            return false;

        return true;
    }
}
