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

        // ---- 보스 아레나: "boss":{"arena":{"x":..,"y":..,"w":..,"h":..}} ----
        // 중첩 오브젝트만 별도 정규식으로 추출
        {
            // boss.arena 블록의 내부 텍스트를 캡처
            static const std::regex boss_re (
                R"REGEX("boss"\s*:\s*\{[^}]*"arena"\s*:\s*\{([^}]*)\})REGEX" ,
                std::regex::icase
            );
            std::smatch bm;
            if ( std::regex_search ( s , bm , boss_re ) ) {
                const std::string inner = bm[ 1 ].str ( ); // arena {...} 내부
                auto find_int = [ & ] ( const char* k , int& outv )->bool {
                    std::regex r ( std::string ( "\"" ) + k + R"("\s*:\s*([-+]?[0-9]+))" );
                    std::smatch mm;
                    if ( std::regex_search ( inner , mm , r ) ) { outv = std::stoi ( mm[ 1 ].str ( ) ); return true; }
                    return false;
                    };
                int bx = 0 , by = 0 , bw = 0 , bh = 0;
                const bool ok =
                    find_int ( "x" , bx ) &&
                    find_int ( "y" , by ) &&
                    find_int ( "w" , bw ) &&
                    find_int ( "h" , bh );
                if ( ok ) {
                    out.has_boss_arena = true;
                    out.boss_x = bx; out.boss_y = by; out.boss_w = bw; out.boss_h = bh;
                }
            }
        }

        // 필수 키 검증
        if ( out.tileset.empty ( ) || out.tiledefs.empty ( ) || out.tilemap.empty ( )
            || out.monsters.empty ( ) || out.player_start.empty ( ) || out.background.empty ( ) )
            return false;

        return true;
    }
}
