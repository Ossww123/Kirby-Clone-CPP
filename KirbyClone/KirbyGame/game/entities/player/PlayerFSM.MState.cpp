// game/entities/player/PlayerFSM.Move.cpp
//
// Responsibility: Movement-track states (Idle/Walk/Run/Crouch/Slide/Jump/Fall/Inflated/Ladder).
// Non-Goals:      Action/overlay logic, core stepping/integration.
// Call-Context:   Invoked from PlayerFSM::Step (fixed step).
//
#include "engine/util/Anim.h"
#include "game/entities/player/PlayerFSM.h"
#include <algorithm> // std::max
#include <cmath>     // std::abs

namespace { inline void Play ( engine::Animator* a , const char* n , bool reset = true ) { if ( a ) a->Play ( n , reset ); } }

namespace game {

    // ===== Grounded / Airborne base =====
    void PlayerFSM::M_Grounded::Update ( Ctx& c , PlayerFSM& f )
    {
        const float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // Leave ground → Fall
        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( !stable ) {
            f.m_fallT = 0.f; f.m_fellFromJump = false; f.m_tumbleT = 0.f;
            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            return;
        }

        // Down → Crouch
        if ( c.ay < -0.5f ) {
            if ( f.m_mState != MState::Crouch && f.m_mState != MState::Slide )
                f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch );
        }
    }

    void PlayerFSM::M_Airborne::Update ( Ctx& c , PlayerFSM& f )
    {
        const float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // Start Inflate in air with jump
        if ( ( f.m_mState == MState::Jump || f.m_mState == MState::Fall ) && c.jumpPressed ) {
            f.RequestMove ( std::make_unique<M_Inflated> ( ) , MState::Inflated );
        }

        // Landed (and not rising)
        const bool stable = ( c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f ) ) && ( c.vel.y >= 0.f );
        if ( stable ) {
            // Long-fall bounce
            if ( f.m_bounceQueued ) {
                auto v = c.body->Velocity ( );
                v.y = -std::abs ( f.m_cfg.bounceSpeedUp );
                c.body->SetVelocity ( v );
                f.m_bounceQueued = false;
                f.m_inLongFall = false;
                f.m_fallT = 0.f; f.m_tumbleT = 0.f;
                Play ( c.anim , "Bounce" , true );
                return; // stay airborne this tick
            }
            if ( std::abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
            else                                    f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    // ===== Idle / Walk / Run / Crouch / Slide =====
    void PlayerFSM::M_Idle::OnEnter ( Ctx& c ) { Play ( c.anim , "Idle" , true ); }
    void PlayerFSM::M_Idle::Update ( Ctx& c , PlayerFSM& f )
    {
        c.body->SetDesiredRunAxis ( 0.f );
        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( !stable ) { f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall ); return; }

        if ( c.ay < -0.5f ) { f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch ); return; }
        if ( std::abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
    }

    void PlayerFSM::M_Walk::OnEnter ( Ctx& c ) { Play ( c.anim , "Walk" , true ); }
    void PlayerFSM::M_Walk::Update ( Ctx& c , PlayerFSM& f )
    {
        M_Grounded::Update ( c , f );
        if ( f.m_mState != MState::Walk ) return;

        if ( std::abs ( c.ax ) <= RUN_TOGGLE_AX ) { f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle ); return; }

        // Double-tap → Run
        if ( f.m_runQueued ) {
            f.m_runQueued = false;
            f.RequestMove ( std::make_unique<M_Run> ( ) , MState::Run );
        }
    }

    void PlayerFSM::M_Run::OnEnter ( Ctx& c ) { Play ( c.anim , "Run" , true ); }
    void PlayerFSM::M_Run::Update ( Ctx& c , PlayerFSM& f )
    {
        M_Grounded::Update ( c , f );
        if ( f.m_mState != MState::Run ) return;

        if ( std::abs ( c.ax ) < 0.75f ) { f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk ); return; }
        if ( c.ay < -0.5f ) { f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch ); return; }
    }

    void PlayerFSM::M_Crouch::OnEnter ( Ctx& c ) { Play ( c.anim , "Crouch" , true ); }
    void PlayerFSM::M_Crouch::Update ( Ctx& c , PlayerFSM& f )
    {
        c.body->SetDesiredRunAxis ( 0.f );

        // Slide on jump/attack
        if ( c.jumpPressed || c.attackPressed ) {
            f.m_slideT = SLIDE_TIME;
            auto v = c.body->Velocity ( ); v.x = static_cast< float >( f.m_facing ) * SLIDE_VX;
            c.body->SetVelocity ( v );
            f.RequestMove ( std::make_unique<M_Slide> ( ) , MState::Slide );
            return;
        }

        if ( c.ay >= -0.5f ) f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
    }

    void PlayerFSM::M_Slide::OnEnter ( Ctx& c ) { Play ( c.anim , "Slide" , true ); }
    void PlayerFSM::M_Slide::Update ( Ctx& c , PlayerFSM& f )
    {
        // Keep momentum; ignore input
        c.body->SetDesiredRunAxis ( static_cast< float >( f.m_facing ) );

        if ( f.m_slideT <= 0.f ) {
            if ( c.ay < -0.5f ) f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch );
            else                f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    // ===== Jump / Fall / Inflated / Ladder =====
    void PlayerFSM::M_Jump::OnEnter ( Ctx& c ) { Play ( c.anim , "Jump" , true ); }
    void PlayerFSM::M_Jump::Update ( Ctx& c , PlayerFSM& f )
    {
        const float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        if ( f.m_jumpLockT > 0.f ) return;

        // Jump → Inflate (tap jump)
        if ( c.jumpPressed ) { f.RequestMove ( std::make_unique<M_Inflated> ( ) , MState::Inflated ); return; }

        if ( c.body->Velocity ( ).y >= 0.f ) {
            f.m_fallT = 0.f; f.m_fellFromJump = true; f.m_tumbleT = f.m_cfg.fallTumbleMs;
            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            return;
        }

        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( stable ) {
            if ( std::abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
            else                                     f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    void PlayerFSM::M_Fall::OnEnter ( Ctx& ) {}
    void PlayerFSM::M_Fall::Update ( Ctx& c , PlayerFSM& f )
    {
        const float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // First frame: choose Fall0/Fall1
        if ( f.m_fallT <= 0.f ) {
            Play ( c.anim , ( f.m_tumbleT > 0.f ? "Fall0" : "Fall1" ) , true );
            f.m_inLongFall = false; f.m_bounceQueued = false;
            f.m_fallY0 = static_cast< float >( c.aabb.b );
        }

        // Phase timers
        f.m_fallT += c.dt;
        if ( f.m_tumbleT > 0.f ) {
            f.m_tumbleT = std::max ( 0.f , f.m_tumbleT - c.dt );
            if ( f.m_tumbleT <= 0.f ) Play ( c.anim , "Fall1" , true );
        }

        // Long-fall entry (time OR drop height)
        if ( !f.m_inLongFall ) {
            const float dropPx = static_cast< float >( c.aabb.b ) - f.m_fallY0;
            if ( f.m_fallT >= f.m_cfg.fallLongMs || dropPx >= f.m_cfg.fallLongHeightPx ) {
                f.m_inLongFall = true;
                Play ( c.anim , "Fall2" , true );
            }
        }

        // Land/bounce routing in base
        M_Airborne::Update ( c , f );
    }

    void PlayerFSM::M_Inflated::OnEnter ( Ctx& c ) { Play ( c.anim , "Inflate" , true ); }
    void PlayerFSM::M_Inflated::Update ( Ctx& c , PlayerFSM& f )
    {
        const float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // Flap (tap jump) → small lift
        if ( c.jumpPressed ) {
            auto v = c.body->Velocity ( );
            v.y = -240.f;
            if ( v.y > -240.f ) v.y = -240.f;
            c.body->SetVelocity ( v );
        }

        // Soft-fall clamp
        auto v = c.body->Velocity ( );
        constexpr float MAX_FALL_VY = 80.f;
        if ( v.y > MAX_FALL_VY ) { v.y = MAX_FALL_VY; c.body->SetVelocity ( v ); }
    }

    void PlayerFSM::M_Ladder::OnEnter ( Ctx& ) {}
    void PlayerFSM::M_Ladder::Update ( Ctx& , PlayerFSM& ) {}

} // namespace game
