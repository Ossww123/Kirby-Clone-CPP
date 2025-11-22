//
// Responsibility: Parse simple CSV files for stage data (player start, monsters, tile defs, doors, tilemap, layers).
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

    struct ItemCSV {
        std::string type;
        float x = 0.f;
        float y = 0.f;
    };

    // Tile definition row as authored in CSV.
    // id         : map tile id
    // solid      : 0/1
    // oneway     : 0/1
    // water      : 0/1 (player enters water state)
    // ladder     : 0/1 (climbable)
    // star_block : 0/1 (breakable star block)
    // slope      : small int code (0:none, 1:45L, 2:45R, 3:22L, 4:22R)
    // gx,gy      : atlas grid indices (0-based; -1 if unspecified)
    struct TileDefCSV {
        int id{ 0 };
        int solid{ 0 };
        int oneway{ 0 };

        int water{ 0 };
        int ladder{ 0 };
        int star_block{ 0 };
        int slope{ 0 };

        int gx{ -1 };
        int gy{ -1 };
    };

    struct DoorCSV {
        int x{ 0 } , y{ 0 } , w{ 0 } , h{ 0 }; // world-pixel AABB
        std::string target;         // stage path to travel to
    };

    struct SpawnCSV {
        std::string name;
        float x{ 0.f } , y{ 0.f };
        int   dir{ +1 };  // -1 or +1
    };

    struct UnlockCSV {
        std::string require;  // stageId to be cleared, e.g., "t1/s1/m1"
        int tx{ 0 } , ty{ 0 } , w{ 0 } , h{ 0 }; // tile-rect on cover layer
    };

    // Tile layer row (runtime tile layer description).
    // name        : layer identifier (for editor/debug UI)
    // tileset     : path to atlas PNG
    // tiledefs    : path to tile definition CSV
    // tilemap     : path to tile-id grid CSV
    // offset_px_x : world offset in pixels (layer origin)
    // offset_px_y : world offset in pixels (layer origin)
    // collides    : 1/0 (whether this layer should build solid/one-way colliders)
    // z           : draw order (smaller = background)
    struct TileLayerCSV {
        std::string name;
        std::string tileset;
        std::string tiledefs;
        std::string tilemap;
        int offsetPxX{ 0 };
        int offsetPxY{ 0 };
        int collides{ 1 };
        int z{ 0 };
    };

    // ---- Loaders ----
    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out );
    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out );
    bool LoadItemsCSV ( const char* path , std::vector<ItemCSV>& out );
    bool LoadTileDefsCSV ( const char* path , std::vector<TileDefCSV>& out );
    bool LoadTileMapCSV ( const char* path , int& outW , int& outH , std::vector<int>& outIds );
    bool LoadDoorsCSV ( const char* path , std::vector<DoorCSV>& out );
    bool LoadSpawnsCSV ( const char* path , std::vector<SpawnCSV>& out );
    bool LoadUnlocksCSV ( const char* path , std::vector<UnlockCSV>& out );
    bool LoadTileLayersCSV ( const char* path , std::vector<TileLayerCSV>& out );

} // namespace game
