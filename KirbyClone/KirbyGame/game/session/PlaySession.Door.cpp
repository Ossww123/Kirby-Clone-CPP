// PlaySession.Door.cpp
//
// Responsibility: Door interaction and stage transition (fade-out → load → fade-in).
// Non-Goals    : Asset loading policy beyond calling LoadStage.
// Call-Context : Fixed update tick drives the transition state machine.

#include "game/session/PlaySession.h"
#include "game/entities/player/Player.h"

namespace {
    // Local overlap for engine::IntRect to avoid extra collision includes.
    inline bool OverlapIR ( const engine::IntRect& a , const engine::IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }
}

namespace game {

    void PlaySession::StartTransitionTo ( const std::string& target , float o , float i )
    {
        if ( target.empty ( ) ) return;
        m_trans = {};
        m_trans.state = Transition::FadingOut;
        m_trans.target = target;
        m_trans.fadeOut = o;
        m_trans.fadeIn = i;
        StartFadeOut ( o );
    }

    bool PlaySession::checkDoorInteract ( )
    {
        if ( m_trans.state != Transition::Idle ) return false; // ignore while transitioning
        if ( !m_Player || m_Doors.empty ( ) ) return false;

        int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
        engine::IntRect paabb{ px, py, px + pw, py + ph };

        for ( const auto& d : m_Doors ) {
            engine::IntRect daabb{ d.x, d.y, d.x + d.w, d.y + d.h };
            if ( OverlapIR ( paabb , daabb ) ) {
                StartTransitionTo ( d.target );
                return true;
            }
        }
        return false;
    }

    void PlaySession::updateTransition ( double /*fixedDt*/ )
    {
        switch ( m_trans.state ) {
        case Transition::Idle:
            break;

        case Transition::FadingOut:
            if ( !IsFading ( ) ) {
                m_trans.state = Transition::Loading;
                // Load target stage, then fade in
                LoadStage ( m_trans.target.c_str ( ) );
                StartFadeIn ( m_trans.fadeIn );
                m_trans.state = Transition::FadingIn;
            }
            break;

        case Transition::FadingIn:
            if ( !IsFading ( ) ) m_trans.state = Transition::Idle;
            break;

        case Transition::Loading:
        default:
            break;
        }
    }

} // namespace game
