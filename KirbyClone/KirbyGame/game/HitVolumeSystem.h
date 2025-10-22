#pragma once
//
// Responsibility: Manage multiple HitVolume instances end-to-end:
//                 - Spawn from archetypes (via HitVolumeFactory)
//                 - Follow owners (anchor & facing)
//                 - Overlap tests vs entity AABBs (circle/box/capsule approximations)
//                 - Emit hit/despawn events, enforce per-target gating
// Non-Goals:      - World tile collision; Projectile rendering/logic
//                 - Resource loading (only optional debug draw)
// Call-Context:   - Main thread only; fixed update loop
//

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <windows.h>
#include "engine/Math.h"
#include "engine/D3D11DebugDraw.h"
#include "game/HitVolume.h"
#include "game/CombatTarget.h"      // CombatTarget

namespace game {

    class HitVolumeFactory; // fwd

    class HitVolumeSystem {
    public:
        using Target = CombatTarget;

        struct HitEvent {
            int        targetId = -1;
            int        volumeId = -1;
            int        ownerId  = -1;
            HitPayload payload{};
        };

        struct DespawnEvent {
            int  volumeId = -1;
            bool natural = true; // ttl or manual kill
        };

        struct SpawnDesc {
            std::string archetype;  // e.g., "SparkAura" / "BeamSweep"
            int         ownerId = -1;
            int         ownerFacing = +1;
            engine::Vec2 worldAnchor{ 0.f,0.f }; // initial anchor (will be updated by locator if attached)
        };

        // The system calls this to ask: where is the owner now? what's its facing?
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

        int Spawn ( const SpawnDesc& s ); // returns volume id or -1

        void Step ( double fixedDt , const std::vector<Target>& targets );

        void DrainHitEvents ( std::vector<HitEvent>& out ) {
            out.insert ( out.end ( ) , m_hits.begin ( ) , m_hits.end ( ) );
            m_hits.clear ( );
        }
        void DrainDespawnEvents ( std::vector<DespawnEvent>& out ) {
            out.insert ( out.end ( ) , m_despawns.begin ( ) , m_despawns.end ( ) );
            m_despawns.clear ( );
        }

        void DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ) const;

    private:
        struct Slot {
            int id = -1;
            std::unique_ptr<HitVolume> hv;
            // cached params for fast sweep (Beam)
            engine::Vec2 prevA{} , prevB{};
            float prevR{ 0.f };
            bool  hasPrev{ false };
        };

        // Geometry helpers
        static bool Overlap_RectCircle ( const RECT& r , const engine::Vec2& c , float rad );
        static bool Overlap_SegmentAABB ( const engine::Vec2& p0 , const engine::Vec2& p1 , const RECT& aabb );
        static bool Overlap_RectCapsule ( const RECT& r , const engine::Vec2& p0 , const engine::Vec2& p1 , float radius );

        // Build world-shape for this volume at current time
        void BuildShape ( const HitVolume& hv , /*out*/ RECT& outBox ,
                        /*out*/ engine::Vec2& segA , /*out*/ engine::Vec2& segB ,
                        /*out*/ float& outRadius ) const;

    private:
        std::vector<Slot> m_vols;
        std::vector<HitEvent> m_hits;
        std::vector<DespawnEvent> m_despawns;
        int m_nextId = 1;

        OwnerLocatorFn m_locateOwner;
    };

} // namespace game
