#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace game {

    struct PlayerStartCSV {
        float x = 64.f , y = 64.f; int dir = 1;
    };

    struct MonsterCSV {
        std::string type;
        float x = 0.f , y = 0.f; int dir = 1;
        // === Dee 공통 이동 옵션(미지정: -1) ===
        int   turnOnHitX = -1;   // 0/1
        int   turnAtEdge = -1;   // 0/1
        
        // === Doo 사격 옵션(미지정: 음수) ===
        float wakeRange = -1.f;
        float windupMs = -1.f;
        float firePeriod = -1.f;
        float bulletSpeed = -1.f;
        int   stopDuringWindup = -1; // 0/1
    };

    bool LoadPlayerStartCSV ( const char* path , PlayerStartCSV& out );   // path: ".../player_start.csv"
    bool LoadMonstersCSV ( const char* path , std::vector<MonsterCSV>& out ); // path: ".../monsters.csv"

} // namespace game
