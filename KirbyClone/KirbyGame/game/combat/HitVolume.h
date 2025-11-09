//
// Responsibility: A single damage/capture hit volume with lifetime, per-target gating,
//                 and optional owner attachment (position + facing).
// Non-Goals:      World tile collision, rendering/I-O, multi-instance mgmt.
// Call-Context:   Main thread, fixed-step update only. No dynamic allocs in Update().
//
#pragma once

#include <unordered_map>
#include "engine/core/Object.h"
#include "engine/util/Math.h"         // Vec2
#include "game/combat/Ability.h"      // Ability enum

namespace engine { class Input; }

namespace game {

    enum class HitShape { Box , Circle , Capsule };
    enum class HitBehavior { Attached , AreaPulse , MeleeArc };
    enum class HitEffect { Damage , Capture }; // Capture: inhale/instant remove event

    struct HitPayload {
        HitEffect    effect{ HitEffect::Damage };
        int          damage{ 1 };                 // for Damage
        engine::Vec2 knockback{ 0.f, 0.f };       // for Damage
        Ability      gift{ Ability::None };       // for Capture (fallback to target meta if None)
    };

    class HitVolume final : public engine::Object {
    public:
        struct Cfg {
            // Behavior & shape
            HitBehavior behavior{ HitBehavior::Attached };
            HitShape    shape{ HitShape::Box };

            // Shape params
            float w{ 16.f } , h{ 8.f };    // Box
            float r{ 24.f };              // Circle
            float len{ 64.f } , thick{ 10.f }; // Capsule (len=centerline, radius=thick*0.5)

            // Lifetime / timing
            float ttl{ 0.25f };
            float armTime{ 0.0f };
            int   tickIntervalMs{ 120 };  // AreaPulse: min interval per target
            // MeleeArc sweep
            float startDeg{ -40.f };
            float endDeg{ +55.f };
            float sweepDuration{ 0.20f };

            // Attachment
            bool         followFacing{ true };
            engine::Vec2 localOffset{ 0.f, 0.f };

            // Combat payload
            HitPayload payload{};

            // Rules
            bool perTargetOnce{ false };
            bool excludeOwner{ true };
        };

    public:
        HitVolume ( int ownerEntityId ,
                  int ownerFacing ,
                  const engine::Vec2& worldAnchor ,
                  const Cfg& cfg );

        void Update ( double fixedDt , const engine::Input& ) override;
        void Advance ( float dt ) noexcept;

        // ---- Queries (system use) ----
        [[nodiscard]] bool         Alive ( )   const noexcept { return m_alive; }
        [[nodiscard]] float        TTL ( )     const noexcept { return m_ttl; }
        [[nodiscard]] float        Age ( )     const noexcept { return m_time; }
        [[nodiscard]] int          Owner ( )   const noexcept { return m_owner; }
        [[nodiscard]] int          Facing ( )  const noexcept { return m_facing; }
        [[nodiscard]] engine::Vec2 Anchor ( )  const noexcept { return m_anchor; }
        [[nodiscard]] const Cfg&   GetCfg ( )  const noexcept { return m_cfg; }

        void SetFacing ( int f ) noexcept { m_facing = ( f >= 0 ) ? +1 : -1; }
        void SetAnchor ( const engine::Vec2& p ) noexcept { m_anchor = p; }

        // Per-target hit gating
        bool CanHitTarget ( int targetId ) const;
        void MarkHitTarget ( int targetId );

    private:
        int          m_owner{ -1 };
        int          m_facing{ +1 };
        engine::Vec2 m_anchor{ 0.f, 0.f };
        Cfg          m_cfg{};

        bool  m_alive{ false };
        float m_ttl{ 0.f };
        float m_time{ 0.f };

        // per-target last-hit age (seconds since spawn)
        mutable std::unordered_map<int , float> m_lastHitAge;
    };

} // namespace game
