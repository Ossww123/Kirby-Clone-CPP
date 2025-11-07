#pragma once
// Responsibility: Lightweight projectile object that owns its PhysicsBody and per-shot state.
// Non-Goals    : No rendering or pooling; no asset ownership.
// Call-Context : Spawned by gameplay systems; updated each fixed tick.

#include "engine/core/Object.h"          // base class (needs full type)
#include "engine/physics/PhysicsBody.h"     // member by value (needs full type)
#include "engine/util/Math.h"            // engine::Vec2
#include "engine/util/Types.h"         // engine::IntRect (no Windows RECT)
#include "game/combat/CombatTypes.h"       // game::ProjOwner

// Forward decls to minimize header coupling.
namespace engine { class Input; }
namespace engine::physics { class CollisionSystem; }

namespace game {

    // Optional hit payload stored by the projectile (read-only to others).
    struct ProjPayload {
        int damage = 1;
        engine::Vec2 knockback{ 0.f, 0.f };
    };

    // A single projectile instance. Owns its PhysicsBody and per-shot state.
    class Projectile final : public engine::Object {
    public:
        // Fixed attributes copied from an archetype at spawn time.
        struct Cfg {
            // Collision (AABB in world pixels)
            float width = 8.f;
            float height = 8.f;

            // Kinematics
            float speed = 480.f;     // convenience (Fire() helpers may use this)
            float ttl = 1.5f;      // seconds

            // World interaction
            bool  dieOnAnyWorldHit = true; // vanish on any solid/ceiling/floor contact

            // Physics overrides (applied to PhysicsBody::Params on construction)
            float gravity = 0.f;
            float frictionAir = 0.f;
            float frictionGround = 0.f;
            float termVel = 99999.f;

            // One-way platforms: if true, always ignore; if false, ignore only while rising
            bool  ignoreOneWay = true;
        };

        // Construction: world bounds define clamping/kill-outside rules in PhysicsBody.
        Projectile ( const engine::IntRect& worldBounds ,
                     const engine::physics::CollisionSystem* col ,
                     ProjOwner owner ,
                     const Cfg& cfg = {} ) noexcept;

        // Activate with position and initial velocity.
        void Fire ( const engine::Vec2& pos , const engine::Vec2& vel ) noexcept {
            m_body.SetPosition ( pos.x , pos.y );
            m_body.SetVelocity ( vel );
            m_alive = true;
            m_ttl = m_cfg.ttl;
        }

        // ---- engine::Object ----
        void Update ( double fixedDt , const engine::Input& ) override;

        // ---- Queries / Controls ----
        [[nodiscard]] bool       Alive ( )   const noexcept { return m_alive; }
        [[nodiscard]] ProjOwner  Owner ( )   const noexcept { return m_owner; }
        [[nodiscard]] const Cfg& GetCfg ( )  const noexcept { return m_cfg; }
        [[nodiscard]] Cfg& GetCfg ( )        noexcept { return m_cfg; } // rare runtime tweak

        void Kill ( ) noexcept { m_alive = false; }

        void GetBounds ( int& x , int& y , int& w , int& h ) const noexcept {
            m_body.GetBounds ( x , y , w , h );
        }

        [[nodiscard]] engine::Vec2 Velocity ( ) const noexcept { return m_body.Velocity ( ); }
        void SetVelocity ( const engine::Vec2& v ) noexcept { m_body.SetVelocity ( v ); }

        // Optional damage payload (for systems that want projectile-owned data)
        void SetPayload ( const ProjPayload& p ) noexcept { m_payload = p; }
        [[nodiscard]] const ProjPayload& Payload ( ) const noexcept { return m_payload; }

    private:
        void Advance ( float dt ) noexcept;

    private:
        // Physics & collision
        engine::PhysicsBody m_body;
        const engine::physics::CollisionSystem* m_col = nullptr;

        // Config & ownership
        Cfg       m_cfg{};
        ProjOwner m_owner{ ProjOwner::Player };

        // Runtime state
        bool  m_alive{ false };
        float m_ttl{ 0.f };

        // Optional hit data
        ProjPayload m_payload{};
    };

} // namespace game
