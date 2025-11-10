// game/entities/player/PlayerFSM.Overlay.cpp
//
// Responsibility: Overlay-track states (Damaged/Dead/DoorEnter/Dance/GameOver) and interact.
// Non-Goals:      Movement/action logic, physics.
// Call-Context:   Invoked from PlayerFSM::Step (fixed step).
//
#include "engine/util/Anim.h"
#include "game/entities/player/PlayerFSM.h"
#include <algorithm> // std::max

namespace { inline void Play ( engine::Animator* a , const char* n , bool reset = true ) { if ( a ) a->Play ( n , reset ); } }

namespace game {

    void PlayerFSM::Z_None::Update ( Ctx& c , PlayerFSM& f )
    {
        // Interact only when no overlay is active
        if ( c.interactPressed ) {
            PlayerEvent ev{ PlayerEvent::DoorInteract };
            ev.rect = c.aabb;
            ev.facing = f.m_facing;
            f.m_events.push_back ( ev );
        }
    }

    void PlayerFSM::Z_Damaged::OnEnter ( Ctx& c ) { Play ( c.anim , "Hurt" , true ); }
    void PlayerFSM::Z_Damaged::Update ( Ctx& c , PlayerFSM& f )
    {
        c.mod.lockRunAxis = true;

        // Cancel Inflate on damage
        if ( f.m_mState == MState::Inflated ) {
            const bool grounded = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
            if ( grounded ) f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
            else            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
        }

        f.m_damagedT = std::max ( 0.f , f.m_damagedT - c.dt );
        if ( f.m_damagedT <= 0.f ) f.RequestOver ( std::make_unique<Z_None> ( ) , ZState::None );
    }

    void PlayerFSM::Z_Dead::OnEnter ( Ctx& c ) { Play ( c.anim , "Death" , true ); }
    void PlayerFSM::Z_Dead::Update ( Ctx& c , PlayerFSM& ) { c.mod.lockRunAxis = true; }

    void PlayerFSM::Z_DoorEnter::OnEnter ( Ctx& c ) { Play ( c.anim , "DoorEnter" , true ); }
    void PlayerFSM::Z_DoorEnter::Update ( Ctx& , PlayerFSM& ) {}

    void PlayerFSM::Z_Dance::OnEnter ( Ctx& c ) { Play ( c.anim , "Dance" , true ); }
    void PlayerFSM::Z_Dance::Update ( Ctx& , PlayerFSM& ) {}

    void PlayerFSM::Z_GameOver::OnEnter ( Ctx& c ) { Play ( c.anim , "GameOver" , true ); }
    void PlayerFSM::Z_GameOver::Update ( Ctx& , PlayerFSM& ) {}

} // namespace game
