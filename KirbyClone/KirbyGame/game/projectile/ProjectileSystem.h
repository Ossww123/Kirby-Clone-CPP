#pragma once
//
// Responsibility: Manage multiple projectile instances end-to-end:
//                 - Spawn from archetypes (via ProjectileFactory)
//                 - Fixed-step update and world collision (delegates to each Projectile)
//                 - Entity overlap tests and hit event emission
//                 - Lifetime & cleanup
// Non-Goals:      - Resource loading / rendering (only optional debug draw)
//                 - Hit resolution on victims (we only emit events)
//                 - Melee/Hitbox management (separate HitVolumeSystem later)
// Call-Context:   - Main thread only; fixed update loop
//                 - Avoid per-step heap churn if possible (reserve vectors); Create() may allocate
//

#include <memory>
#include <string>
#include <vector>

#include "game/Projectile.h"
#include "game/ProjectileFactory.h"
#include "engine/Collision.h"      // Overlap()
#include "engine/Math.h"
#include "game/CombatTarget.h"      // CombatTarget

namespace engine { class D3D11DebugDraw; }

namespace game {

    class ProjectileSystem {
    public:
        // One “victim candidate” the system can test against.
        // Game layer가 각 프레임마다 대상 목록을 만들어 전달.
        using Target = CombatTarget;

        // Emitted when a projectile overlaps a valid target.
        struct HitEvent {
            int          targetId = -1;
            int          projectileId = -1; // system-local handle (unique per spawn)
            ProjOwner    owner = ProjOwner::Player;
            ProjPayload  payload{};         // damage/knockback
            engine::Vec2 incomingDir{ 0,0 };  // projectile velocity at hit
            RECT         projectileAabb{ 0,0,0,0 };
        };

        // Optional: spawn convenience descriptor (archetype-driven)
        struct SpawnDesc {
            std::string     archetype;      // e.g., "Star"
            ProjOwner       owner = ProjOwner::Player;
            engine::Vec2    pos{ 0,0 };
            engine::Vec2    dirOrVel{ 1,0 };  // if not normalized, treated as raw velocity
            bool            treatAsDirection = true; // true: dir * speed, false: use as velocity
        };

    public:
        ProjectileSystem ( ) = default;

        void Initialize ( const RECT& world , const engine::physics::CollisionSystem* col ) {
            m_world = world;
            m_col = col;
            m_slots.clear ( );
            m_hits.clear ( );
            m_slots.reserve ( 64 );
            m_hits.reserve ( 64 );
        }

        void Clear ( ) {
            m_slots.clear ( );
            m_hits.clear ( );
            m_nextId = 1;
        }

        // Spawn from archetype (via ProjectileFactory). Returns a system-local projectile id; -1 on failure.
        int Spawn ( const SpawnDesc& s );

        // Fixed-step tick: updates all projectiles and performs entity overlap tests.
        void Step ( double fixedDt , const std::vector<Target>& targets );

        // Pull and clear hit events (Game layer consumes and applies damage)
        void DrainHitEvents ( std::vector<HitEvent>& out ) {
            out.insert ( out.end ( ) , m_hits.begin ( ) , m_hits.end ( ) );
            m_hits.clear ( );
        }

        // Optional debug draw: draw projectile AABBs
        void DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ) const;

        // Stats / queries
        int  ActiveCount ( ) const { return static_cast< int >( m_slots.size ( ) ); }

    private:
        struct Slot {
            int id = -1;
            std::unique_ptr<Projectile> pr;
        };

        // Helpers
        static bool TeamAllowsHit ( ProjOwner owner , bool targetIsPlayer ) {
            if ( owner == ProjOwner::Player ) return !targetIsPlayer; // player shots hit enemies
            return  targetIsPlayer;                                  // enemy shots hit player
        }

    private:
        RECT m_world{ 0,0,0,0 };
        const engine::physics::CollisionSystem* m_col = nullptr;

        std::vector<Slot>     m_slots;
        std::vector<HitEvent> m_hits;

        int m_nextId = 1;
    };

} // namespace game
