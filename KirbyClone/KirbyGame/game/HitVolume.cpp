#include "game/HitVolume.h"

namespace game {

    bool HitVolume::CanHitTarget ( int targetId ) const {
        if ( m_cfg.perTargetOnce ) {
            return ( m_lastHitAge.find ( targetId ) == m_lastHitAge.end ( ) ) && ( m_time >= m_cfg.armTime );
        }
        // time-gated (tickIntervalMs)
        const auto it = m_lastHitAge.find ( targetId );
        if ( m_time < m_cfg.armTime ) return false;
        if ( it == m_lastHitAge.end ( ) ) return true;
        const float interval = ( m_cfg.tickIntervalMs <= 0 ) ? 0.f : ( m_cfg.tickIntervalMs / 1000.f );
        return ( m_time - it->second ) >= interval;
    }

    void HitVolume::MarkHitTarget ( int targetId ) {
        m_lastHitAge[ targetId ] = m_time;
    }

} // namespace game
