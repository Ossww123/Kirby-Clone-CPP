// Responsibility: Implementation of projectile update, hit detection and debug draw.
// Non-Goals    : Rendering sprites or audio; pooling.
// Call-Context : Used by PlaySession fixed-step.

#include "game/projectile/ProjectileSystem.h"
#include "game/projectile/ProjectileFactory.h"
#include "game/debugdraw/ProjectileDebugDraw.h"
#include "engine/render/IDebugDraw.h"
#include "engine/platform/win32/ColorUtil.h"
#include "engine/core/Input.h"
#include "game/combat/CombatTarget.h"   // target fields for Step()

#include <algorithm>

namespace game {

    // local AABB overlap for IntRect
    static inline bool OverlapIR ( const engine::IntRect& a , const engine::IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }

    int ProjectileSystem::Spawn ( const SpawnDesc& s ) {
        auto p = ProjectileFactory::Create ( s.archetype , m_world , m_col , s.owner );
        if ( !p ) return -1;

        engine::Vec2 vel = s.dirOrVel;
        if ( s.treatAsDirection ) {
            // dirOrVel is direction; scale by archetype speed
            const float spd = p->GetCfg ( ).speed;
            vel = { s.dirOrVel.x * spd , s.dirOrVel.y * spd };
        }

        p->Fire ( s.pos , vel );

        Slot slot;
        slot.id = m_nextId++;
        slot.pr = std::move ( p );

        m_slots.emplace_back ( std::move ( slot ) );
        return m_slots.back ( ).id;
    }

    void ProjectileSystem::Step ( double fixedDt , const std::vector<Target>& targets ) {
        static engine::Input kNullInput{};

        // 1) Update
        for ( auto& s : m_slots ) {
            s.pr->Update ( fixedDt , kNullInput ); // ← 캐스트 제거, 참조 안전
        }

        // 2) Entity overlap hits
        for ( auto& s : m_slots ) {
            auto& pr = *s.pr;
            if ( !pr.Alive ( ) ) continue;

            int x , y , w , h; pr.GetBounds ( x , y , w , h );
            engine::IntRect prBox{ x , y , x + w , y + h };

            for ( const auto& t : targets ) {
                if ( !t.alive ) continue;
                if ( !TeamAllowsHit ( pr.Owner ( ) , t.isPlayer ) ) continue;

                if ( OverlapIR ( prBox , t.aabb ) ) {
                    // Emit event; Game layer applies damage/knockback
                    HitEvent ev;
                    ev.targetId = t.id;
                    ev.projectileId = s.id;
                    ev.owner = pr.Owner ( );
                    ev.payload = pr.Payload ( );
                    ev.incomingDir = pr.Velocity ( );
                    ev.projectileAabb = prBox;
                    m_hits.emplace_back ( ev );

                    // Kill on first hit (no pierce in V1)
                    pr.Kill ( );
                    break;
                }
            }
        }

        // 3) Cleanup dead
        m_slots.erase (
            std::remove_if ( m_slots.begin ( ) , m_slots.end ( ) ,
                [ ] ( const Slot& s ) { return !s.pr || !s.pr->Alive ( ); } ) ,
            m_slots.end ( ) );
    }

    void ProjectileSystem::DebugDraw ( engine::IDebugDraw & dbg , int ox , int oy ) const {
        for ( const auto& s : m_slots ) {
            if ( !s.pr || !s.pr->Alive ( ) ) continue;
            ProjectileDebugDraw::Draw ( *s.pr , &dbg , ox , oy );
        }
    }

} // namespace game
