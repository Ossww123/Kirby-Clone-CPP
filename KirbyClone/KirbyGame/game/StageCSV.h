#pragma once
#include <string>
#include <vector>

namespace game {

    struct PlayerStartCSV {
        float x = 64.f , y = 64.f;
        int   dir = 1;
    };

    struct MonsterCSV {
        std::string type;
        float x = 0 , y = 0;
        int   dir = 1;

        // Optional AI params (−1 = unspecified)
        int   turnOnHitX = -1;  // 0/1
        int   turnAtEdge = -1;  // 0/1
        float wakeRange = -1.f;
        float windupMs = -1.f;
        float firePeriod = -1.f;
        float bulletSpeed = -1.f;
        int   stopDuringWindup = -1;  // 0/1
    };

    // Tile definition row as authored in CSV.
    // id: map tile id
    // solid/oneway: 0/1
    // gx,gy: atlas grid coordinates (0-based, cell size units; -1 if unspecified)
    struct TileDefCSV {
        int id = 0;
        int solid = 0;
        int oneway = 0;
        int gx = -1;
        int gy = -1;
    };

    // ---- Loaders ----
    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out );
    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out );
    bool LoadTileDefsCSV ( const char* path , std::vector<TileDefCSV>& out );

    // TileMap 그리드 로더 (숫자 그리드만; 헤더 없음/주석(#/;) 허용)
    // outW*outH == outIds.size() 여야 true
    bool LoadTileMapCSV ( const char* path , int& outW , int& outH , std::vector<int>& outIds );
}
