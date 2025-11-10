#pragma once
//
// Responsibility: Hold stage resource paths and parse a flat JSON descriptor.
// Non-Goals:      Full JSON compliance (objects/arrays beyond simple keys), schema validation.
// Call-Context:   Main thread; small utility; no platform dependencies.
//
#include <string>

namespace game {

    struct StageDesc {
        // identity/spawn
        std::string id;               // e.g., "t1/s1/m2"
        std::string spawns;           // "assets/.../spawns.csv" (multi spawn points)

        // core assets
        std::string tileset;          // assets/tilesets/grasslands.png
        std::string tiledefs;         // assets/tilesets/grasslands.tiledefs.csv
        std::string tilemap;          // assets/stages/.../tilemap.csv
        std::string monsters;         // assets/stages/.../monsters.csv
        std::string player_start;     // assets/stages/.../player_start.csv
        std::string background;       // assets/backgrounds/sky_day.png
        std::string doors;            // optional: assets/stages/.../doors.csv

        // hub-only (optional)
        std::string cover_tilemap;    // cover layer tilemap csv
        std::string unlocks;          // unlock rules csv

        // optional boss arena
        bool has_boss_arena{ false };
        int  boss_x{ 0 } , boss_y{ 0 } , boss_w{ 0 } , boss_h{ 0 };
    };

    // Parse a very simple JSON with flat key-value pairs: "key":"value" or "key": number.
    // Returns false if required keys are missing.
    bool LoadStageDesc ( const char* jsonPath , StageDesc& out );

} // namespace game
