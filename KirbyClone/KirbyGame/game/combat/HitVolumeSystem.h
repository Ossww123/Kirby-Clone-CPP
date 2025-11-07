//
// Responsibility: Manage lifetimes, owner-following, collision tests, and events for HitVolumes.
// Non-Goals:      Rendering logic beyond optional debug draw; persistence; multithread sync.
// Call-Context:   Main thread only; fixed-step update.
//
#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "engine/util/Types.h"          // IntRect
#include "engine/util/Math.h"           // Vec2
#include "engine/core/Object.h"         // engine::Object base (Update signature)
#include "game/combat/HitVolume.h"      // HitVolume, HitPayload, enums
#include "game/combat/CombatTarget.h"   // CombatTarget (Target alias)

namespace engine { 
    class D3D11DebugDraw;
    class IDebugDraw;
}

namespace game {

    class HitVolumeFactory;

    class HitVolumeSystem {
    public:
        using Target = CombatTarget;

        struct HitEvent {
            int        targetId{ -1 };
            int        volumeId{ -1 };
            int        ownerId{ -1 };
            HitPayload payload{};
            Ability    gift{ Ability::None };
        };

        struct DespawnEvent {
            int  volumeId{ -1 };
            bool natural{ true }; // ttl or manual kill
        };

        struct SpawnDesc {
            std::string  archetype;              // "SparkAura", "BeamSweep", "InhaleField", ...
            int          ownerId{ -1 };
            int          ownerFacing{ +1 };
            engine::Vec2 worldAnchor{ 0.f, 0.f };

            // --- Optional overrides (position policy) ---
            bool         overrideOffset{ false };
            engine::Vec2 localOffset{};          // if overrideOffset==true, replaces cfg.localOffset

            bool         overrideFollowFacing{ false };
            bool         followFacing{ true };   // if overrideFollowFacing==true, replaces cfg.followFacing
        };

        // System asks the world: where is the owner? what is its facing?
        using OwnerLocatorFn = std::function<bool ( int ownerId , engine::Vec2& outAnchor , int& outFacing )>;

    public:
        HitVolumeSystem ( ) = default;

        void SetOwnerLocator ( OwnerLocatorFn fn ) { m_locateOwner = std::move ( fn ); }

        void Initialize ( ) {
            m_vols.clear ( );
            m_hits.clear ( );
            m_despawns.clear ( );
            m_nextId = 1;
            m_vols.reserve ( 32 );
        }

        void Clear ( ) { Initialize ( ); }

        [[nodiscard]] int Spawn ( const SpawnDesc& s ); // returns volume id or -1

        void Step ( double fixedDt , const std::vector<Target>& targets );

        void DrainHitEvents ( std::vector<HitEvent>& out ) {
            out.insert ( out.end ( ) , m_hits.begin ( ) , m_hits.end ( ) );
            m_hits.clear ( );
        }
        void DrainDespawnEvents ( std::vector<DespawnEvent>& out ) {
            out.insert ( out.end ( ) , m_despawns.begin ( ) , m_despawns.end ( ) );
            m_despawns.clear ( );
        }

        void DebugDraw ( engine::IDebugDraw& dbg , int ox , int oy ) const;

    private:
        struct Slot {
            int id{ -1 };
            std::unique_ptr<HitVolume> hv;
            // cached params for swept capsule (Beam)
            engine::Vec2 prevA{} , prevB{};
            float prevR{ 0.f };
            bool  hasPrev{ false };
        };

    private:
        std::vector<Slot>        m_vols;
        std::vector<HitEvent>    m_hits;
        std::vector<DespawnEvent> m_despawns;
        int                      m_nextId{ 1 };

        OwnerLocatorFn           m_locateOwner;
    };

} // namespace game
