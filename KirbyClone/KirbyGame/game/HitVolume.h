#pragma once
//
// Responsibility: A single damage "hit volume" (e.g., Spark aura / Beam sweep)
//                 with lifetime, per-target hit rules, and optional attachment to an owner.
// Non-Goals:      - World tile collision (that’s Projectile’s job)
//                 - Resource loading / rendering (only optional debug draw bounds)
//                 - Multi-instance management (use HitVolumeSystem)
// Call-Context:   - Main thread, fixed update loop only
//                 - No dynamic allocations in Update()
//

#include <windows.h>
#include <unordered_map>
#include "engine/Object.h"
#include "engine/Math.h"
#include "game/Ability.h"

namespace game {

    enum class HitShape { Box , Circle , Capsule };
    enum class HitBehavior { Attached , AreaPulse , MeleeArc };

    enum class HitEffect { Damage , Capture }; // Capture = 흡입/즉시 제거 이벤트
    
    struct HitPayload {
        HitEffect    effect{ HitEffect::Damage };
        int          damage{ 1 };            // Damage용
        engine::Vec2 knockback{ 0.f, 0.f };  // Damage용
        Ability      gift{ Ability::None };  // Capture용(없으면 타깃 메타에서 가져옴)
    };

    class HitVolume final : public engine::Object {
    public:
        struct Cfg {
            // Behavior & shape
            HitBehavior behavior = HitBehavior::Attached;
            HitShape    shape = HitShape::Box;

            // Shape params (interpretation depends on shape)
            // Box: w,h | Circle: r | Capsule: len (centerline), thick (diameter -> radius = thick*0.5)
            float w = 16.f , h = 8.f;
            float r = 24.f;
            float len = 64.f , thick = 10.f;

            // Lifetime / timing
            float ttl = 0.25f;     // seconds
            float armTime = 0.0f;  // seconds before it can hit
            // AreaPulse
            int   tickIntervalMs = 120; // minimal interval between hits on the same target
            // MeleeArc (sweep from startDeg to endDeg over duration)
            float startDeg = -40.f;
            float endDeg = +55.f;
            float sweepDuration = 0.20f;

            // Attachment
            bool         followFacing = true;
            engine::Vec2 localOffset{ 0.f, 0.f };

            // Combat
            HitPayload payload{};

            // Rules
            bool perTargetOnce = false; // if true, one hit per life regardless of tickInterval
            bool excludeOwner = true;
        };

    public:
        HitVolume ( int ownerEntityId , int ownerFacing , const engine::Vec2& worldAnchor , const Cfg& cfg )
            : m_owner ( ownerEntityId ) , m_facing ( ownerFacing ) , m_anchor ( worldAnchor ) , m_cfg ( cfg )
        {
            m_alive = true;
            m_ttl = cfg.ttl;
            m_time = 0.f;
        }

        void Update ( double fixedDt , const engine::Input& ) override {
            if ( !m_alive ) return;
            const float dt = static_cast< float >( fixedDt );
            m_time += dt;
            m_ttl -= dt;
            if ( m_ttl <= 0.f ) m_alive = false;
        }

        // ---- Queries (for system) ----
        bool  Alive ( )     const { return m_alive; }
        float TTL ( )       const { return m_ttl; }
        float Age ( )       const { return m_time; }
        int   Owner ( )     const { return m_owner; }
        int   Facing ( )    const { return m_facing; }
        void  SetFacing ( int f ) { m_facing = ( f >= 0 ) ? +1 : -1; }
        void  SetAnchor ( const engine::Vec2& p ) { m_anchor = p; }
        engine::Vec2 Anchor ( ) const { return m_anchor; }
        const Cfg& GetCfg ( ) const { return m_cfg; }

        // Hit-gating: remember last-hit time per target
        bool CanHitTarget ( int targetId ) const;
        void MarkHitTarget ( int targetId );

    private:
        int         m_owner{ -1 };
        int         m_facing{ +1 };
        engine::Vec2 m_anchor{ 0.f,0.f };
        Cfg         m_cfg{};

        bool  m_alive{ false };
        float m_ttl{ 0.f };
        float m_time{ 0.f };

        // per-target last-hit age (seconds since spawn)
        mutable std::unordered_map<int , float> m_lastHitAge;
    };

} // namespace game
