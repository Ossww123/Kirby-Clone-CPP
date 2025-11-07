//
// Responsibility: Registry implementation for HitVolume presets.
// Non-Goals:      External synchronization; file I/O; editor tooling.
// Call-Context:   Main thread only.
//
#include "game/combat/HitVolumeFactory.h"
#include <unordered_map>

namespace game {
    namespace {
        using RegistryMap = std::unordered_map<std::string , HitVolume::Cfg>;
        RegistryMap& Registry ( ) {
            static RegistryMap r;
            return r;
        }
    }

    void HitVolumeFactory::Register ( const std::string& id , const HitVolume::Cfg& cfg ) {
        if ( id.empty ( ) ) return;
        Registry ( )[ id ] = cfg;
    }

    const HitVolume::Cfg* HitVolumeFactory::Find ( const std::string& id ) {
        auto& r = Registry ( );
        auto it = r.find ( id );
        return ( it == r.end ( ) ) ? nullptr : &it->second;
    }

    void HitVolumeFactory::RegisterDefaults ( ) {
        // Spark aura: circle centered at owner, ticks every 120ms
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::AreaPulse;
            c.shape = HitShape::Circle;
            c.r = 46.f;
            c.ttl = 0.90f;
            c.armTime = 0.04f;
            c.tickIntervalMs = 120;
            c.followFacing = true;
            c.localOffset = { 0.f, 0.f };
            c.payload.effect = HitEffect::Damage;
            c.payload.damage = 1;
            c.payload.knockback = { 80.f, -60.f };
            c.perTargetOnce = false;
            c.excludeOwner = true;
            HitVolumeFactory::Register ( "SparkAura" , c );
        }

        // Beam sweep: capsule sweeping from -40deg to +55deg over ~0.28s
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::MeleeArc;
            c.shape = HitShape::Capsule;
            c.len = 300.f;
            c.thick = 100.f;
            c.startDeg = -40.f;
            c.endDeg = +55.f;
            c.sweepDuration = 0.28f;
            c.ttl = 0.30f;   // margin beyond sweep
            c.armTime = 0.04f;
            c.followFacing = true;
            // c.localOffset = { 10.f, -6.f }; // from player's hand (tweakable)
            c.localOffset = { 0.f, 0.f };   // neutral; hand offset comes from SpawnDesc
            c.payload.effect = HitEffect::Damage;
            c.payload.damage = 2;
            c.payload.knockback = { 260.f, -90.f };
            c.perTargetOnce = true;       // single hit per swing
            c.excludeOwner = true;
            HitVolumeFactory::Register ( "BeamSweep" , c );
        }

        // Inhale field: short-lived attached box at Kirby's mouth; capture-only
        {
            HitVolume::Cfg c{};
            c.behavior = HitBehavior::Attached;
            c.shape = HitShape::Box;
            c.w = 48.f; c.h = 20.f;
            c.ttl = 0.08f;      // respawned every frame while inhaling
            c.armTime = 0.f;
            c.followFacing = true;
            // c.localOffset = { 18.f, -2.f }; // mouth position (tuning point)
            c.localOffset = { 0.f, 0.f };   // neutral; mouth offset via SpawnDesc
            c.payload.effect = HitEffect::Capture;
            c.payload.gift = Ability::None; // gift chosen from target meta if None
            c.perTargetOnce = true;
            c.excludeOwner = true;
            HitVolumeFactory::Register ( "InhaleField" , c );
        }
    }

} // namespace game
