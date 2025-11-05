#include "game/ProjectileSystem.h"
#include "engine/D3D11DebugDraw.h"

namespace game {

    int ProjectileSystem::Spawn ( const SpawnDesc& s ) {
        auto p = ProjectileFactory::Create ( s.archetype , m_world , m_col , s.owner );
        if ( !p ) return -1;

        engine::Vec2 vel = s.dirOrVel;
        if ( s.treatAsDirection ) {
            // dirOrVel is direction; scale by archetype speed
            const float spd = p->GetCfg ( ).speed;
            vel = { s.dirOrVel.x * spd, s.dirOrVel.y * spd };
        }

        p->Fire ( s.pos , vel );

        Slot slot;
        slot.id = m_nextId++;
        slot.pr = std::move ( p );

        m_slots.emplace_back ( std::move ( slot ) );
        return m_slots.back ( ).id;
    }

    void ProjectileSystem::Step ( double fixedDt , const std::vector<Target>& targets ) {
        // 1) Update
        for ( auto& s : m_slots ) {
            s.pr->Update ( fixedDt , /*input*/ *( engine::Input* )nullptr ); // Projectile ignores Input
            // NOTE: 위 캐스트는 Update 시그니처 요구 충족용. 내부에서 Input 사용 안 함.
        }

        // 2) Entity overlap hits
        for ( auto& s : m_slots ) {
            auto& pr = *s.pr;
            if ( !pr.Alive ( ) ) continue;

            int x , y , w , h; pr.GetBounds ( x , y , w , h );
            RECT prBox{ x,y,x + w,y + h };

            for ( const auto& t : targets ) {
                if ( !t.alive ) continue;
                if ( !TeamAllowsHit ( pr.Owner ( ) , t.isPlayer ) ) continue;

                if ( engine::physics::Overlap ( prBox , t.aabb ) ) {
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

    void ProjectileSystem::DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ) const {
        for ( const auto& s : m_slots ) {
            if ( !s.pr || !s.pr->Alive ( ) ) continue;
            int x , y , w , h; s.pr->GetBounds ( x , y , w , h );
            dbg.WorldRect ( x , y , w , h , ox , oy , RGB ( 255 , 230 , 0 ) );
        }
    }

} // namespace game
