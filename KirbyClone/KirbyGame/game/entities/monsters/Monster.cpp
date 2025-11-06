//
// Responsibility: Monster base impl — AI hook, physics/collision, health.
// Non-Goals:      Rendering/Debug draw (handled by external adapters).
// Call-Context:   Main thread.
//

#include "game/entities/monsters/Monster.h"

#include "engine/core/Object.h"             // GenEntityId()
#include "engine/core/Input.h"
#include "engine/util/Math.h"               // Vec2
#include "engine/util/Types.h"              // IntRect
#include "engine/physics/Collision.h"       // CollisionSystem/Report, Overlap

#include "game/combat/Ability.h"            // Ability default

namespace game {

    Ability Monster::AbilityGift ( ) const { return Ability::None; }

    Monster::Monster ( const engine::IntRect& worldBounds ,
                     const engine::physics::CollisionSystem* col ,
                     const Cfg& cfg )
        : m_body ( worldBounds , cfg.phys )
        , m_col ( col )
        , m_cfg ( cfg )
    {
        SetId ( engine::GenEntityId ( ) );
        m_health.Reset ( cfg.maxHp , cfg.iFrameMs );
    }

    void Monster::Update ( double fixedDt , const engine::Input& input ) {
        if ( !m_alive ) return;

        // 1) AI decides desired motion/state
        TickAI ( fixedDt , input );

        // 2) Physics/collision
        StepPhysics ( fixedDt );

        // 3) Animation advance
        m_anim.Update ( fixedDt );

        // 4) Invulnerability timer
        m_health.Tick ( static_cast< float >( fixedDt ) );
    }

    void Monster::OnHit ( const Damage& d ) {
        if ( !m_alive ) return;

        const bool took = m_health.Apply ( d.amount ); // true: hp changed, false: i-frames or dead

        if ( m_health.hp <= 0 ) { // always check even if Apply returned false
            m_alive = false;
            return; // no knockback on death
        }
        if ( !took ) return;

        // knockback
        auto v = m_body.Velocity ( );
        v.x += d.knockback.x * m_cfg.knockbackMul;
        v.y += d.knockback.y * m_cfg.knockbackMul;
        m_body.SetVelocity ( v );
    }

    engine::Vec2 Monster::Center ( ) const {
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        return { x + w * 0.5f, y + h * 0.5f };
    }

    int Monster::Facing ( ) const {
        return ( m_body.Velocity ( ).x >= 0.f ) ? +1 : -1;
    }

    void Monster::StepPhysics ( double fixedDt ) {
        // 1) integrate accel/gravity
        m_body.AdvanceKinematics ( fixedDt );

        // 2) predict AABB & normals
        int   prevBottom = 0;
        float nx = 0.f , ny = 0.f;
        engine::IntRect aabb = m_body.ProposeAABB ( fixedDt , &prevBottom , &nx , &ny );

        engine::Vec2 vel = m_body.Velocity ( );
        m_ignoreOneWay = ( m_cfg.ignoreOneWayUpward && vel.y < 0.f );

        // 3) collide & apply
        engine::physics::CollisionReport rep{};
        m_col->MoveAndCollide ( aabb , vel , &rep , m_ignoreOneWay , prevBottom );
        m_body.ApplyCollisionResult ( aabb , vel , rep , nx , ny );

        // expose minimal collision info to derived classes
        m_lastHitX = rep.hitX;
        m_lastHitY = rep.hitY;
    }

    bool Monster::HasGroundAhead ( int dir ) const {
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const int probeW = 2;
        const int px0 = ( dir > 0 ) ? ( x + w + 1 ) : ( x - probeW - 1 );
        const int px1 = ( dir > 0 ) ? ( x + w + 1 + probeW ) : ( x - 1 );
        engine::IntRect probe{ px0, y + h, px1, y + h + 3 };

        // test vs statics and one-ways
        for ( const auto& s : m_col->Statics ( ) ) if ( engine::physics::Overlap ( probe , s ) ) return true;
        for ( const auto& o : m_col->OneWays ( ) ) if ( engine::physics::Overlap ( probe , o ) ) return true;
        return false;
    }

} // namespace game
