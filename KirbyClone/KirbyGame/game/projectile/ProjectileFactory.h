#pragma once
// Responsibility: Define projectile archetypes and create Projectile instances by id.
// Non-Goals    : Rendering, pooling, or asset loading.
// Call-Context : Gameplay systems register/lookup archetypes and spawn projectiles.

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/util/Types.h"      // engine::IntRect
#include "engine/util/Math.h"       // engine::Vec2
#include "game/combat/CombatTypes.h"// game::ProjOwner (fixed underlying type)

namespace engine::physics { class CollisionSystem; } // fwd
namespace game { class Projectile; }                  // fwd

namespace game {

    // Data preset for one projectile kind (archetype)
    struct ProjDef {
        // Collision AABB
        float width = 8.f;
        float height = 8.f;

        // Kinematics / lifetime
        float speed = 480.f;
        float ttl = 1.5f;

        // Physics overrides
        float gravity = 0.f;
        float frictionAir = 0.f;
        float frictionGround = 0.f;
        float termVel = 99999.f;

        // World interaction
        bool  dieOnAnyWorldHit = true;
        bool  ignoreOneWay = true;

        // Combat payload (optional; injected into Projectile as read-only payload)
        int   damage = 1;
        engine::Vec2 knockback{ 0.f, 0.f };

        // Visual keys (optional; ids only)
        // std::string spriteSheetId;
        // std::string animClipName;
    };

    class ProjectileFactory {
    public:
        // Register/lookup presets
        static void Register ( const std::string& id , const ProjDef& d );
        static const ProjDef* Find ( const std::string& id );

        // Useful presets (Star, AirPuff, FirePellet)
        static void RegisterDefaults ( );

        // Create a single projectile instance from an archetype id
        static std::unique_ptr<Projectile> Create (
            const std::string& id ,
            const engine::IntRect& worldRect ,
            const engine::physics::CollisionSystem* col ,
            ProjOwner owner
        );

        // Load presets from CSV (columns are optional; missing -> keep defaults)
        // Supported headers (case-sensitive):
        //   id,width,height,speed,ttl,gravity,frictionAir,frictionGround,termVel,
        //   dieOnAnyWorldHit,ignoreOneWay,damage,knockbackX,knockbackY
        static bool LoadCSV ( const char* filename );

    private:
        static std::unordered_map<std::string , ProjDef>& Registry ( );
    };

} // namespace game
