#pragma once
//
// Responsibility: Data-driven archetype registry for HitVolume (Spark aura / Beam sweep, etc.)
//                 - Keep map<string, HitVolume::Cfg>
//                 - Provide built-in defaults (SparkAura, BeamSweep)
// Non-Goals:      - Runtime management (use HitVolumeSystem)
//                 - Resource loading/rendering
// Call-Context:   - Main thread only
//

#include <string>
#include <unordered_map>
#include "game/HitVolume.h"

namespace game {

    class HitVolumeFactory {
    public:
        static void Register ( const std::string& id , const HitVolume::Cfg& cfg );
        static const HitVolume::Cfg* Find ( const std::string& id );
        static void RegisterDefaults ( ); // SparkAura, BeamSweep

    private:
        static std::unordered_map<std::string , HitVolume::Cfg>& Registry ( );
    };

} // namespace game
