#include "game/combat/HitVolume.h"

namespace game {

    HitVolume::HitVolume ( int ownerEntityId ,
                         int ownerFacing ,
                         const engine::Vec2& worldAnchor ,
                         const Cfg& cfg )
        : m_owner ( ownerEntityId )
        , m_facing ( ownerFacing >= 0 ? +1 : -1 )
        , m_anchor ( worldAnchor )
        , m_cfg ( cfg )
        , m_alive ( true )
        , m_ttl ( cfg.ttl )
        , m_time ( 0.f )
    {}

    void HitVolume::Update(double fixedDt, const engine::Input&) { Advance((float)fixedDt); }

    void HitVolume::Advance(float dt) noexcept {
        if (!m_alive) return;
        m_time += dt;
        m_ttl -= dt;
        if (m_ttl <= 0.f) m_alive = false;
    }

    bool HitVolume::CanHitTarget ( int targetId ) const {
        if ( m_time < m_cfg.armTime ) return false;

        if ( m_cfg.perTargetOnce ) {
            return m_lastHitAge.find ( targetId ) == m_lastHitAge.end ( );
        }

        const auto it = m_lastHitAge.find ( targetId );
        if ( it == m_lastHitAge.end ( ) ) return true;

        const float interval = ( m_cfg.tickIntervalMs <= 0 )
            ? 0.f
            : static_cast< float >( m_cfg.tickIntervalMs ) / 1000.f;
        return ( m_time - it->second ) >= interval;
    }

    void HitVolume::MarkHitTarget ( int targetId ) {
        m_lastHitAge[ targetId ] = m_time;
    }

} // namespace game
