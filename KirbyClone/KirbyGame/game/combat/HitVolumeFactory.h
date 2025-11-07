//
// Responsibility: Register/find named HitVolume presets (string id → HitVolume::Cfg).
// Non-Goals:      Thread safety beyond single-thread loop; persistence or hot-reload UI.
// Call-Context:   Main thread. Used by abilities/systems to spawn volumes by id.
//
#pragma once

#include <string>
#include "game/combat/HitVolume.h"

namespace game {

    class HitVolumeFactory {
    public:
        static void Register ( const std::string& id , const HitVolume::Cfg& cfg );
        [[nodiscard]] static const HitVolume::Cfg* Find ( const std::string& id );
        static void RegisterDefaults ( ); // SparkAura, BeamSweep, InhaleField
    };

} // namespace game
