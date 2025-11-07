//
// Responsibility: Parse simple CSV files for stage data (player start, monsters, tile defs, doors, tilemap).
// Non-Goals:      Full CSV quoting/escaping; schema validation beyond minimal checks.
// Call-Context:   Main thread; plain file I/O; header keeps minimal deps.
//
#pragma once
#include <string>
#include <vector>

namespace game {

    struct PlayerStartCSV {
        float x{ 64.f } , y{ 64.f };
        int   dir{ 1 };
    };

    struct MonsterCSV {
        std::string type;
        float x{ 0.f } , y{ 0.f };
        int   dir{ 1 };     // -1, 0, +1
        int   attack{ 1 };  // 1/0
        int   move{ 1 };    // 1/0
    };

    // Tile definition row as authored in CSV.
    // id: map tile id
    // solid/oneway: 0/1
    // gx,gy: atlas grid indices (0-based; -1 if unspecified)
    struct TileDefCSV {
        int id{ 0 };
        int solid{ 0 };
        int oneway{ 0 };
        int gx{ -1 };
        int gy{ -1 };
    };

    struct DoorCSV {
        int x{ 0 } , y{ 0 } , w{ 0 } , h{ 0 }; // world-pixel AABB
        std::string target;         // stage path to travel to
    };

    // ---- Loaders ----
    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out );
    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out );
    bool LoadTileDefsCSV ( const char* path , std::vector<TileDefCSV>& out );
    bool LoadDoorsCSV ( const char* path , std::vector<DoorCSV>& out );

    // TileMap grid loader (numeric grid only; no header; comments #/; allowed).
    // Returns true iff outW*outH == outIds.size().
    bool LoadTileMapCSV ( const char* path , int& outW , int& outH , std::vector<int>& outIds );

} // namespace game
