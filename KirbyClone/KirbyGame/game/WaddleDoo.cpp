#include "game/WaddleDoo.h"
#include "engine/Math.h"
#include "game/Projectile.h"

using namespace engine;

namespace game {

    static inline float Len ( const Vec2& v ) { return std::sqrt ( v.x * v.x + v.y * v.y ); }
    static inline Vec2   Norm ( const Vec2& v ) {
        const float l = Len ( v ); if ( l <= 1e-6f ) return { 0,0 }; return { v.x / l, v.y / l };
    }

    void WaddleDoo::TickAI ( double fixedDt , const engine::Input& )
    {
        // 이동 없음(정지 사수). 필요 시 m_body.SetDesiredRunAxis(±1)로 패트롤 추가 가능.
        m_body.SetDesiredRunAxis ( 0.f );

        // 쿨다운
        m_cd = std::max ( 0.f , m_cd - ( float ) fixedDt );

        // 타겟 없으면 패스
        if ( !m_queryTarget ) return;

        // 타겟 위치/발사 조건
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        Vec2 myCenter{ x + w * 0.5f, y + h * 0.5f };
        Vec2 target = m_queryTarget ( );
        Vec2 to = { target.x - myCenter.x, target.y - myCenter.y };
        if ( Len ( to ) > m_cfg.wakeRange ) return;

        if ( m_cd <= 0.f && m_spawnProj ) {
            Vec2 dir = Norm ( to );
            Vec2 vel = { dir.x * m_cfg.bulletSpeed, dir.y * m_cfg.bulletSpeed };
            // 살짝 눈높이에서 발사
            Vec2 muzzle{ myCenter.x, myCenter.y - 6.f };

            m_spawnProj ( muzzle , vel , ProjOwner::Enemy );
            m_cd = m_cfg.firePeriod;
        }
    }

} // namespace game
