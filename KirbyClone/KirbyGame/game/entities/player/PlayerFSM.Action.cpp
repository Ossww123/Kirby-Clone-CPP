//
// Responsibility: Action-track states (Inhale, MouthFull, Spit, AirPuff, AbilityAtk).
// Non-Goals:      Movement/overlay logic, physics/collision.
// Call-Context:   Invoked from PlayerFSM::Step (fixed step).
//

#include "engine/util/Anim.h"
#include "game/entities/player/PlayerFSM.h"
#include <algorithm> // std::max

namespace { inline void Play ( engine::Animator* a , const char* n , bool reset = true ) { if ( a ) a->Play ( n , reset ); } }

namespace game {

    // ===== Action root =====
    void PlayerFSM::A_Neutral::Update ( Ctx& c , PlayerFSM& f )
    {
        // Priority: (1) spit item (2) air puff (3) ability attack (4) inhale
        if ( f.m_mouthFull && c.attackPressed ) { f.RequestAct ( std::make_unique<A_SpitObject> ( ) , AState::SpitObject ); return; }
        if ( f.m_mState == MState::Inflated && c.attackPressed ) { f.RequestAct ( std::make_unique<A_AirPuff> ( ) , AState::AirPuff ); return; }
        if ( f.m_ability != Ability::None && c.attackPressed ) {
            f.RequestAct ( std::make_unique<A_AbilityAtk> ( ) , AState::AbilityAtk ); return;
        }
        // Guard: no inhale while crouching/sliding or holding down
        const bool crouchLike = ( f.m_mState == MState::Crouch || f.m_mState == MState::Slide || c.ay < -0.5f );
        if ( !f.m_mouthFull && c.attackHeld && f.m_ability == Ability::None && !crouchLike ) {
            f.RequestAct ( std::make_unique<A_Inhale> ( ) , AState::Inhale ); return;
        }
    }

    void PlayerFSM::A_Inhale::OnEnter ( Ctx& c ) { Play ( c.anim , "Inhale" , true ); }
    void PlayerFSM::A_Inhale::Update ( Ctx& c , PlayerFSM& f )
    {
        // Cancel if crouch/slide starts
        if ( f.m_mState == MState::Crouch || f.m_mState == MState::Slide ) {
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
            return;
        }

        // Slow walk while inhaling
        c.mod.runAxisMul = 0.5f;

        // Hold timer
        if ( f.m_inhaleT <= 0.f ) f.m_inhaleT = 0.55f;
        f.m_inhaleT = std::max ( 0.f , f.m_inhaleT - c.dt );

        // Emit inhale volume
        PlayerEvent ev{ PlayerEvent::InhaleVolume };
        ev.rect = f.MakeInhaleBox ( c );
        ev.facing = f.m_facing;
        f.m_events.push_back ( ev );

        // Exit conditions
        if ( f.m_mouthFull ) { f.RequestAct ( std::make_unique<A_MouthFull> ( ) , AState::MouthFull ); return; }
        if ( !c.attackHeld || f.m_inhaleT <= 0.f ) {
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
    }

    void PlayerFSM::A_MouthFull::OnEnter ( Ctx& c ) { Play ( c.anim , "MouthFullIdle" , true ); }
    void PlayerFSM::A_MouthFull::Update ( Ctx& c , PlayerFSM& f )
    {
        if ( !f.m_mouthFull ) { f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral ); return; }

        // X: spit
        if ( c.attackPressed ) { f.RequestAct ( std::make_unique<A_SpitObject> ( ) , AState::SpitObject ); return; }

        // Down: swallow (copy ability)
        if ( c.ay < -0.5f ) {
            PlayerEvent e1{ PlayerEvent::SwallowAbility }; e1.ability = f.m_caughtGift; f.m_events.push_back ( e1 );
            f.m_ability = f.m_caughtGift; f.m_caughtGift = Ability::None;
            f.m_mouthFull = false;
            PlayerEvent e2{ PlayerEvent::AbilityGained }; e2.ability = f.m_ability; f.m_events.push_back ( e2 );
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
    }

    void PlayerFSM::A_SpitObject::OnEnter ( Ctx& c ) { Play ( c.anim , "Spit" , true ); }
    void PlayerFSM::A_SpitObject::Update ( Ctx& c , PlayerFSM& f )
    {
        // One-shot emit
        if ( !f.m_spitEmitted ) {
            PlayerEvent ev{ PlayerEvent::SpitStar };
            ev.facing = f.m_facing;
            f.m_events.push_back ( ev );
            f.m_spitLockT = 0.18f;    // short lock
            f.m_mouthFull = false;
            f.m_caughtGift = Ability::None;
            f.m_spitEmitted = true;
        }

        // Lock movement while firing
        c.mod.lockRunAxis = true;

        // Return to Neutral after lock
        if ( f.m_spitEmitted && f.m_spitLockT <= 0.f ) {
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
    }

    void PlayerFSM::A_AirPuff::OnEnter ( Ctx& c ) { Play ( c.anim , "AirPuff" , true ); }
    void PlayerFSM::A_AirPuff::Update ( Ctx& c , PlayerFSM& f )
    {
        // One-shot emit
        if ( f.m_spitLockT <= 0.f ) {
            PlayerEvent ev{ PlayerEvent::AirPuffShot }; ev.facing = f.m_facing; f.m_events.push_back ( ev );
            f.m_spitLockT = 0.14f;

            // Leave Inflate immediately
            const bool grounded = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
            if ( f.m_mState == MState::Inflated ) {
                if ( grounded ) f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
                else            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            }
        }

        // Lock during puff
        c.mod.lockRunAxis = true;

        // Unlock and exit
        f.m_spitLockT = std::max ( 0.f , f.m_spitLockT - c.dt );
        if ( f.m_spitLockT <= 0.f ) f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
    }

    void PlayerFSM::A_AbilityAtk::OnEnter ( Ctx& c ) { Play ( c.anim , "AbilityAtk" , true ); }
    void PlayerFSM::A_AbilityAtk::Update ( Ctx& c , PlayerFSM& f )
    {
        // One-shot emit per ability
        if ( f.m_spitLockT <= 0.f ) {
            switch ( f.m_ability ) {
            case Ability::Fire: {
                PlayerEvent ev{ PlayerEvent::AbilityFire }; ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.22f;
                break;
            }
            case Ability::Spark: {
                PlayerEvent ev{ PlayerEvent::AbilitySpark }; ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.25f;
                break;
            }
            case Ability::Beam: {
                PlayerEvent ev{ PlayerEvent::AbilityBeam }; ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.18f;
                break;
            }
            default:
                // No ability → exit
                f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
                return;
            }
        }

        // Lock while attacking
        c.mod.lockRunAxis = true;

        // Early release if button up
        if ( !c.attackHeld && f.m_spitLockT > 0.f ) f.m_spitLockT = 0.f;

        // Exit after lock
        f.m_spitLockT = std::max ( 0.f , f.m_spitLockT - c.dt );
        if ( f.m_spitLockT <= 0.f ) f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
    }

} // namespace game
