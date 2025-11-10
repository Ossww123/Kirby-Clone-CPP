//
// Responsibility: Implement the spawn resolution chain.
// Non-Goals:      Player/Scene manipulation; world loading.
// Call-Context:   Main thread.
//
#include "game/session/SpawnSelector.h"
#include <vector>
#include <algorithm>

namespace game {

    static bool find_spawn_by_name ( const char* csv , std::string_view key , ResolvedSpawn& out ) {
        if ( !csv || key.empty ( ) ) return false;
        std::vector<SpawnCSV> rows;
        if ( !LoadSpawnsCSV ( csv , rows ) ) return false;
        auto it = std::find_if ( rows.begin ( ) , rows.end ( ) , [ & ] ( const SpawnCSV& s ) { return s.name == key; } );
        if ( it == rows.end ( ) ) return false;
        out.name = it->name; out.x = it->x; out.y = it->y; out.dir = ( it->dir == -1 ) ? -1 : +1;
        return true;
    }

    bool ResolveSpawn ( const StageDesc& desc , const char* overrideName , const protocol::SaveData* save , ResolvedSpawn& out ) {
        // 1) explicit override
        if ( overrideName && *overrideName ) {
            if ( find_spawn_by_name ( desc.spawns.c_str ( ) , overrideName , out ) ) return true;
        }
        // 2) spawns.csv with lastSpawn (from save)
        if ( save && !save->lastSpawn.empty ( ) ) {
            if ( find_spawn_by_name ( desc.spawns.c_str ( ) , save->lastSpawn , out ) ) return true;
        }
        // 3) spawns.csv "default"
        if ( find_spawn_by_name ( desc.spawns.c_str ( ) , "default" , out ) ) return true;

        // 4) fallback: player_start.csv (single)
        if ( !desc.player_start.empty ( ) ) {
            PlayerStartCSV ps{};
            if ( LoadPlayerStartCSV ( desc.player_start.c_str ( ) , ps ) ) {
                out.name = "player_start"; out.x = ps.x; out.y = ps.y; out.dir = ( ps.dir == -1 ) ? -1 : +1;
                return true;
            }
        }
        return false;
    }

} // namespace game
