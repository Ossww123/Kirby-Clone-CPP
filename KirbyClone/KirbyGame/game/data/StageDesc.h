//
// Responsibility: Hold stage resource paths and parse a flat JSON descriptor.
// Non-Goals:      Full JSON compliance (objects/arrays beyond simple keys), schema validation.
// Call-Context:   Main thread; small utility; no platform dependencies.
//
#pragma once
#include <string>

namespace game {

    struct StageDesc {
        std::string id;          
        std::string hub_spawn;   
        std::string tileset;     
        std::string tiledefs;    
        std::string tilemap;     
        std::string monsters;    
        std::string player_start;
        std::string background;  
        std::string doors;       

        // optional boss arena
        bool has_boss_arena{ false };
        int  boss_x{ 0 } , boss_y{ 0 } , boss_w{ 0 } , boss_h{ 0 };
    };

    // Parse a very simple JSON with flat key-value pairs: "key":"value" or "key": number.
    // Returns false if required keys are missing.
    bool LoadStageDesc ( const char* jsonPath , StageDesc& out );

} // namespace game
