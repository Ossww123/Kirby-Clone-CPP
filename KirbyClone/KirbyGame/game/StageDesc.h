#pragma once
#include <string>

namespace game {
    struct StageDesc {
        std::string tileset;        // assets/tilesets/grasslands.png
        std::string tiledefs;       // assets/tilesets/grasslands.tiledefs.csv
        std::string tilemap;        // assets/stages/stage01/tilemap.csv
        std::string monsters;       // assets/stages/stage01/monsters.csv
        std::string player_start;   // assets/stages/stage01/player_start.csv
        std::string background;     // assets/backgrounds/sky_day.png
        std::string doors;          // optional: assets/stages/stage01/doors.csv
    };

    // 평면 키 전용 초간단 파서: "key":"value" 또는 "key": 123/0.5
    bool LoadStageDesc ( const char* jsonPath , StageDesc& out );
}
