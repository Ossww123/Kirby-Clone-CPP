// PlaySession.Door.cpp

#include "game/PlaySession.h"
#include "engine/Collision.h"
#include "engine/util/Types.h"

namespace game {
    void PlaySession::StartTransitionTo ( const std::string& target , float o , float i )
    {
        if ( target.empty ( ) ) return;
        m_trans = {};
        m_trans.state = Transition::FadingOut;
        m_trans.target = target;
        m_trans.fadeOut = o; m_trans.fadeIn = i;
        StartFadeOut ( o );
    }

    bool PlaySession::checkDoorInteract ( )
    {
        if ( m_trans.state != Transition::Idle ) return false; // 전환 중엔 무시
        if ( !m_Player || m_Doors.empty ( ) ) return false;

        int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
        engine::IntRect paabb{ px,py,px + pw,py + ph };

        for ( const auto& d : m_Doors ) {
            engine::IntRect daabb{ d.x, d.y, d.x + d.w, d.y + d.h };
            if ( engine::physics::Overlap ( paabb , daabb ) ) {
                StartTransitionTo ( d.target );
                return true;
            }
        }
        return false;
    }

    void PlaySession::updateTransition ( double )
    {
        switch ( m_trans.state ) {
        case Transition::Idle: break;
        case Transition::FadingOut:
            if ( !IsFading ( ) ) {
                m_trans.state = Transition::Loading;
                // 실제 로드
                LoadStage ( m_trans.target.c_str ( ) );
                StartFadeIn ( m_trans.fadeIn );
                m_trans.state = Transition::FadingIn;
            }
            break;
        case Transition::FadingIn:
            if ( !IsFading ( ) ) m_trans.state = Transition::Idle;
            break;
        case Transition::Loading: default: break;
        }
    }
} // namespace game
