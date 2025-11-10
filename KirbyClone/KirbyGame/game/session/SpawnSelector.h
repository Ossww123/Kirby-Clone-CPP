#pragma once
//
// Responsibility: Resolve the correct spawn point (override → spawns.csv → save.lastSpawn → default → player_start.csv).
// Non-Goals:      Player/Scene manipulation; world loading.
// Call-Context:   Main thread; called from stage load.
//
#include <string>
#include "protocol/SaveSchema.h"
#include "game/data/StageDesc.h"
#include "game/data/StageCSV.h"

namespace game {

    struct ResolvedSpawn {
        std::string name;   // chosen key (e.g., "door_m2" or "default")
        float x{ 0.f } , y{ 0.f };
        int   dir{ +1 };
    };

    // Return true if a spawn is resolved. Uses fallback chain safely.
    bool ResolveSpawn ( const StageDesc& desc ,
                        const char* overrideName ,          // may be null
                        const protocol::SaveData* save ,    // may be null
                        ResolvedSpawn& out );

} // namespace game
