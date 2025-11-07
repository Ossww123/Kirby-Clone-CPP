#pragma once
#include <memory>
#include <string>
#include <cmath>

#include "engine/Object.h"          // engine::Object (Update/Render interface)
#include "engine/PhysicsBody.h"     // engine::PhysicsBody
#include "engine/Collision.h"       // engine::physics::CollisionSystem/Report
#include "engine/Math.h"            // engine::Vec2
#include "game/CombatTypes.h"       // game::ProjOwner

namespace game {

    // Optional hit payload stored by the projectile (read-only to others).
    struct ProjPayload {
        int damage = 1;
        engine::Vec2 knockback{ 0.f, 0.f };
    };

    // A single projectile instance. Owns its PhysicsBody and per-shot state.
    class Projectile : public engine::Object {
    public:
        // Fixed attributes copied from an archetype at spawn time.
        struct Cfg {
            // Collision (AABB in world pixels)
            float width = 8.f;
            float height = 8.f;

            // Kinematics
            float speed = 480.f;       // convenience (Fire() helpers may use this)
            float ttl = 1.5f;        // seconds

            // World interaction
            bool  dieOnAnyWorldHit = true; // vanish on any solid/ceiling/floor contact

            // Physics overrides (applied to PhysicsBody::Params on construction)
            float gravity = 0.f;
            float frictionAir = 0.f;
            float frictionGround = 0.f;
            float termVel = 99999.f;

            // One-way platforms: if true, always ignore; if false, only ignore while rising
            bool  ignoreOneWay = true;
        };

        // Construction: world bounds define clamping/kill-outside rules in PhysicsBody.
        Projectile ( const RECT& worldBounds ,
                   const engine::physics::CollisionSystem* col ,
                   ProjOwner owner ,
                   const Cfg& cfg = {} ) noexcept
            : m_body ( worldBounds , engine::PhysicsParams{} ) ,
            m_col ( col ) ,
            m_cfg ( cfg ) ,
            m_owner ( owner )
        {
            m_body.SetSize ( m_cfg.width , m_cfg.height );

            // Apply physics overrides from Cfg to the body params.
            auto& p = m_body.Params ( );
            p.gravity = m_cfg.gravity;
            p.frictionAir = m_cfg.frictionAir;
            p.frictionGround = m_cfg.frictionGround;
            p.termVel = m_cfg.termVel;
        }

        // Activate the projectile with position and initial velocity.
        void Fire ( const engine::Vec2& pos , const engine::Vec2& vel ) noexcept {
            m_body.SetPosition ( pos.x , pos.y );
            m_body.SetVelocity ( vel );
            m_alive = true;
            m_ttl = m_cfg.ttl;
        }

        // ---- engine::Object ----
        void Update ( double fixedDt , const engine::Input& ) override {
            if ( !m_alive ) return;

            // Lifetime
            m_ttl -= static_cast< float >( fixedDt );
            if ( m_ttl <= 0.f ) { m_alive = false; return; }

            // Kinematics prior to collision
            m_body.AdvanceKinematics ( fixedDt );

            // Propose move + perform collision resolution against the tile/world colliders
            int   prevBottom = 0;
            float nx = 0.f , ny = 0.f;
            RECT  aabb = m_body.ProposeAABB ( fixedDt , &prevBottom , &nx , &ny );
            auto  vel = m_body.Velocity ( );

            engine::physics::CollisionReport rep{};
            const bool ignoreOneWay = m_cfg.ignoreOneWay ? true : ( vel.y < 0.f ); // pass up-through when rising
            if ( m_col ) {
                m_col->MoveAndCollide ( aabb , vel , &rep , ignoreOneWay , prevBottom );
            }
            m_body.ApplyCollisionResult ( aabb , vel , rep , nx , ny );

            if ( m_cfg.dieOnAnyWorldHit && ( rep.hitX || rep.hitY || rep.grounded ) ) {
                m_alive = false;
            }
        }

        // ---- Queries / Controls ----
        [[nodiscard]] bool       Alive ( )   const noexcept { return m_alive; }
        [[nodiscard]] ProjOwner  Owner ( )   const noexcept { return m_owner; }
        [[nodiscard]] const Cfg& GetCfg ( )  const noexcept { return m_cfg; }
        [[nodiscard]] Cfg& GetCfg ( )        noexcept { return m_cfg; } // allow rare runtime tweak if needed

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
