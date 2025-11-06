//
// Responsibility: WhispyWoods boss AI — puff volley and apple wave.
// Non-Goals:      Rendering/VFX lifetime.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/WhispyWoods.h"

#include <algorithm>
#include "engine/util/Math.h"                     // Vec2::Length()
#include "game/combat/CombatTypes.h"              // ProjOwner
#include "game/entities/monsters/MonsterTypes.h"  // MonsterType, SpawnSpec

namespace game {

    void WhispyWoods::TickAI ( double dt , const engine::Input& ) {
        // stationary: never walks
        m_body.SetDesiredRunAxis ( 0.f );

        m_timer -= static_cast< float >( dt );
        if ( m_timer > 0.f ) return;

        switch ( m_state ) {
        case State::Rest:
            // alternate Puff <-> Apple with rests between
            if ( m_flip ) beginPuffVolley ( );
            else        beginAppleDrop ( );
            m_flip = !m_flip;
            break;

        case State::Puffing:
            if ( m_shotsLeft > 0 ) {
                if ( m_innerT <= 0.f ) {
                    firePuffOnce ( );
                    --m_shotsLeft;
                    m_innerT = m_cfg.puffIntervalMs;
                }
                else {
                    m_innerT -= static_cast< float >( dt );
                }
            }
            else {
                m_state = State::Rest;
                m_timer = m_cfg.puffRestMs;
            }
            break;

        case State::AppleDrop:
            dropApplesOnce ( );
            m_state = State::Rest;
            m_timer = m_cfg.appleRestMs;
            break;
        }
    }

    void WhispyWoods::beginPuffVolley ( ) {
        m_state = State::Puffing;
        m_shotsLeft = m_cfg.puffVolleyCount;
        m_innerT = 0.f; // fire immediately
    }

    void WhispyWoods::firePuffOnce ( ) {
        if ( !m_spawnProjId ) return;

        // fire straight toward player from boss center
        const engine::Vec2 me = Center ( );
        const engine::Vec2 tp = m_queryTarget ? m_queryTarget ( ) : me;
        engine::Vec2 dir{ tp.x - me.x, tp.y - me.y };
        const float len = std::max ( 1.f , dir.Length ( ) );
        dir.x /= len; dir.y /= len;

        const engine::Vec2 vel{ dir.x * m_cfg.puffSpeed, dir.y * m_cfg.puffSpeed };
        m_spawnProjId ( "AirPuff" , me , vel , ProjOwner::Enemy );
    }

    void WhispyWoods::beginAppleDrop ( ) {
        m_state = State::AppleDrop;
        // could add telegraph child here if needed (m_cfg.appleTelegraphMs)
    }

    void WhispyWoods::dropApplesOnce ( ) {
        if ( !m_spawnMonster ) return;

        const engine::Vec2 me = Center ( );
        const float span = m_cfg.appleSpanPx;
        const int   n = std::max ( 1 , m_cfg.applesPerWave );

        for ( int i = 0; i < n; ++i ) {
            const float t = ( n == 1 ) ? 0.f : ( static_cast< float >( i ) / ( n - 1 ) - 0.5f ); // [-0.5, +0.5]
            const float x = me.x + t * span;
            const float y = me.y - 220.f; // drop from above head

            SpawnSpec spec{};
            spec.type = MonsterType::Apple;
            spec.x = x; spec.y = y;
            spec.dir = 0; spec.attack = 1; spec.move = 1;

            m_spawnMonster ( MonsterType::Apple , { x, y } , spec );
        }
    }

} // namespace game
