#pragma once
//
// Responsibility: Data-driven projectile archetype registry and instance creation.
//                 - Keep a map<string, ProjDef> of projectile presets (CSV/JSON or code-registered)
//                 - Map ProjDef -> Projectile::Cfg and construct a single Projectile instance
// Non-Goals:      - Runtime update/render of multiple projectiles (use ProjectileSystem or caller)
//                 - Spawn patterns (burst/spread/fan), pooling, resource loading
//                 - Direct sprite/animation handling (store only keys if needed)
// Call-Context:   - Main thread only
//                 - No dynamic allocation inside tight per-frame loops except Create() by design
//

#include <memory>
#include <string>
#include <unordered_map>

#include "game/Projectile.h"
#include "engine/Collision.h"

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

        // Visual keys (optional; kept as IDs only, real loading is outside)
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
            const RECT& worldRect ,
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
