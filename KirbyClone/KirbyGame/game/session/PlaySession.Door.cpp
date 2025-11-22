// PlaySession.Door.cpp
//
// Responsibility: Door interaction and stage transition (fade-out → load → fade-in).
// Non-Goals    : Asset loading policy beyond calling LoadStage.
// Call-Context : Fixed update tick drives the transition state machine.

#include "game/session/PlaySession.h"
#include "game/entities/player/Player.h"

#include <cwchar> 

#ifndef DBGLOG
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) {
    ::OutputDebugStringW ( msg );
    ::OutputDebugStringW ( L"\n" );
}
#endif

namespace {
    // Local overlap for engine::IntRect to avoid extra collision includes.
    inline bool OverlapIR ( const engine::IntRect& a , const engine::IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }
}

namespace game {

    void PlaySession::StartTransitionTo ( const std::string& target , float o , float i , const char* spawnOverride )
    {
        if ( target.empty ( ) ) return;
        m_trans = {};
        m_trans.state = Transition::FadingOut;
        m_trans.target = target;
        m_trans.fadeOut = o;
        m_trans.fadeIn = i;
        if ( spawnOverride && *spawnOverride ) m_trans.spawn = spawnOverride;
        StartFadeOut ( o );

        // ---- Snapshot savepoint right before leaving this stage ----
        if ( m_Session ) {
            auto & sd = m_Session->MutData ( );
            sd.lastStage = target;
            if ( spawnOverride && *spawnOverride ) sd.lastSpawn = spawnOverride;
            // If we are in hub now, keep hub continuity explicit
            if ( m_stageId == "t1/hub" ) sd.lastHub = "t1/hub";
            ( void ) m_Session->SaveToDisk ( );
        }
    }

    bool PlaySession::checkDoorInteract ( )
    {
        if ( IsClearSequenceActive ( ) ) return false;

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
            //DBGLOG(L"[Trans] Idle");
            break;

        case Transition::FadingOut:
            if ( !IsFading ( ) ) {
                DBGLOG ( L"[Trans] FadingOut done -> Loading" );
                m_trans.state = Transition::Loading;

                // Load target stage, then fade in
                const char* targetJson = m_trans.target.c_str ( );
                const bool ok = LoadStage ( targetJson );

                wchar_t buf[ 256 ];
                std::swprintf (
                    buf , _countof ( buf ) ,
                    L"[Trans] Loading  target=%hs  result=%ls  spawn=%hs" ,
                    targetJson ,
                    ok ? L"OK" : L"FAIL" ,
                    m_trans.spawn.c_str ( )
                );
                DBGLOG ( buf );

                StartFadeIn ( m_trans.fadeIn );
                m_trans.state = Transition::FadingIn;
            }
            break;

        case Transition::FadingIn:
            if ( !IsFading ( ) ) {
                DBGLOG ( L"[Trans] FadingIn done -> Idle" );
                m_trans.state = Transition::Idle;
            }
            break;

        case Transition::Loading:
        default:
            break;
        }
    }

} // namespace game
