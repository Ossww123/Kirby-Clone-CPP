//
// Responsibility: Hold stage resource paths and parse a flat JSON descriptor.
// Non-Goals:      Full JSON compliance (objects/arrays beyond simple keys), schema validation.
// Call-Context:   Main thread; small utility; no platform dependencies.
//
#pragma once
#include <string>

namespace game {

    struct StageDesc {
        std::string tileset;      // assets/tilesets/grasslands.png
        std::string tiledefs;     // assets/tilesets/grasslands.tiledefs.csv
        std::string tilemap;      // assets/stages/stage01/tilemap.csv
        std::string monsters;     // assets/stages/stage01/monsters.csv
        std::string player_start; // assets/stages/stage01/player_start.csv
        std::string background;   // assets/backgrounds/sky_day.png
        std::string doors;        // optional: assets/stages/stage01/doors.csv

        // optional boss arena
        bool has_boss_arena{ false };
        int  boss_x{ 0 } , boss_y{ 0 } , boss_w{ 0 } , boss_h{ 0 };
    };

    // Parse a very simple JSON with flat key-value pairs: "key":"value" or "key": number.
    // Returns false if required keys are missing.
    bool LoadStageDesc ( const char* jsonPath , StageDesc& out );

} // namespace game
