// Responsibility: Out-of-line defs to keep header lean (no heavy includes).
// Non-Goals    : No rendering.
// Call-Context : Linked with gameplay module.

#include "game/projectile/Projectile.h"
#include "engine/core/Input.h"            // used only here
#include "engine/physics/Collision.h"        // engine::physics::CollisionReport

using engine::Vec2;

namespace game {

    Projectile::Projectile ( const engine::IntRect& worldBounds ,
                             const engine::physics::CollisionSystem* col ,
                             ProjOwner owner ,
                             const Cfg& cfg ) noexcept
        : m_body ( worldBounds , engine::PhysicsParams{} )
        , m_col ( col )
        , m_cfg ( cfg )
        , m_owner ( owner )
    {
        m_body.SetSize ( m_cfg.width , m_cfg.height );

        // Apply physics overrides from Cfg to the body params.
        auto& p = m_body.Params ( );
        p.gravity = m_cfg.gravity;
        p.frictionAir = m_cfg.frictionAir;
        p.frictionGround = m_cfg.frictionGround;
        p.termVel = m_cfg.termVel;
    }

    void Projectile::Update ( double fixedDt , const engine::Input& )
    {
        Advance ( static_cast< float >( fixedDt ) );
    }

    void Projectile::Advance ( float dt ) noexcept
    {
        if ( !m_alive ) return;

        // Lifetime
        m_ttl -= dt;
        if ( m_ttl <= 0.f ) { m_alive = false; return; }

        // Kinematics prior to collision
        m_body.AdvanceKinematics ( dt );

        // Propose move + perform collision resolution against the world colliders
        int   prevBottom = 0;
        float nx = 0.f , ny = 0.f;
        auto  aabb = m_body.ProposeAABB ( dt , &prevBottom , &nx , &ny );
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

} // namespace game
