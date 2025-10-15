#pragma once
#include <string>
#include <vector>

namespace game {
    struct PlayerStartCSV { float x = 64.f , y = 64.f; int dir = 1; };

    struct MonsterCSV {
        std::string type; float x = 0 , y = 0; int dir = 1;
        int   turnOnHitX = -1 , turnAtEdge = -1;        // 0/1 (미지정:-1)
        float wakeRange = -1.f , windupMs = -1.f , firePeriod = -1.f , bulletSpeed = -1.f;
        int   stopDuringWindup = -1;                  // 0/1 (미지정:-1)
    };

    struct TileDefCSV { int id = 0; int solid = 0; int oneway = 0; };

    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out );
    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out );
    bool LoadTileDefsCSV ( const char* path , std::vector<TileDefCSV>& out );
}
