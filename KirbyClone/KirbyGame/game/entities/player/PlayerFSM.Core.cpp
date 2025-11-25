//
// Responsibility: PlayerFSM core flow — init/step, collision integrate, transitions, snapshots.
// Non-Goals:      Rendering/audio; per-track state logic (move/action/overlay lives elsewhere).
// Call-Context:   Main thread; called from fixed-step loop.
//

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <cmath>

#include "game/entities/player/PlayerFSM.h"
#include "engine/core/Input.h"
#include "engine/physics/Collision.h"
#include "engine/util/Math.h"
#include "engine/util/Types.h"
#include "engine/util/Anim.h"

namespace game {

    // ===== Public API =====
    void PlayerFSM::Init ( engine::PhysicsBody* body ,
                          const engine::physics::CollisionSystem* worldCol ,
                          engine::Animator* anim ,
                          const Cfg& cfg )
    {
        m_body = body;
        m_col = worldCol;
        m_anim = anim;
        m_cfg = cfg;

        HardResetRuntime ( );
    }

    void PlayerFSM::Step ( double fixedDt , const engine::Input& input )
    {
        if ( !m_body || !m_col ) return;

        Ctx c;
        c.body = m_body; c.col = m_col; c.anim = m_anim; c.cfg = m_cfg;
        c.dt = static_cast< float >( fixedDt );

        // --- input ---
        c.ax = input.GetAxis ( "MoveX" );
        c.ay = input.GetAxis ( "MoveY" );
        c.jumpPressed = input.ActionPressed ( "Jump" );
        c.jumpHeld = input.ActionDown ( "Jump" );
        c.attackPressed = input.ActionPressed ( "Attack" );
        c.attackHeld = input.ActionDown ( "Attack" );
        c.abilityPressed = input.ActionPressed ( "Ability" );
        c.interactPressed = input.ActionPressed ( "Interact" );

        // --- double-tap via axis edge ---
        const float dead = RUN_TOGGLE_AX;

        // edge: cross the deadzone boundary this frame
        const bool leftPressed = ( m_prevAx >= -dead ) && ( c.ax < -dead );
        const bool rightPressed = ( m_prevAx <= dead ) && ( c.ax > dead );
        m_prevAx = c.ax;

        m_tapT = std::max ( 0.f , m_tapT - c.dt );
        auto onTap = [ & ] ( int dir , bool pressed ) {
            if ( !pressed ) return;
            if ( m_lastTapDir == dir && m_tapT > 0.f &&
                ( m_body->Grounded ( ) || m_dbg.groundHoldT > 0.f ) )
                m_runQueued = true;
            m_lastTapDir = dir;
            m_tapT = TAP_WINDOW;
            };
        onTap ( -1 , leftPressed );
        onTap ( +1 , rightPressed );

        // --- timers/health ---
        if ( c.jumpPressed ) m_dbg.bufferT = m_cfg.bufferMs;
        else               m_dbg.bufferT = std::max ( 0.f , m_dbg.bufferT - c.dt );
        m_dbg.coyoteT = std::max ( 0.f , m_dbg.coyoteT - c.dt );
        m_dbg.dropT = std::max ( 0.f , m_dbg.dropT - c.dt );
        m_dbg.groundHoldT = std::max ( 0.f , m_dbg.groundHoldT - c.dt );
        m_jumpLockT = std::max ( 0.f , m_jumpLockT - c.dt );
        m_spitLockT = std::max ( 0.f , m_spitLockT - c.dt );
        m_slideT = std::max ( 0.f , m_slideT - c.dt );
        m_health.Tick ( c.dt );

        // drop-through
        if ( m_body->Grounded ( ) && c.ay < -0.5f && c.jumpPressed )
            m_dbg.dropT = m_cfg.dropMs;

        // physics/collision once
        IntegrateAndCollide ( fixedDt , input , c );

        // facing
        UpdateFacing ( c );

        // ===== Overlay =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_zPending ) { ApplyPendingOver ( c ); continue; }
            if ( m_zNeedEnter && m_overlay ) { m_overlay->OnEnter ( c ); m_zNeedEnter = false; }
            if ( m_overlay ) m_overlay->Update ( c , *this );
            if ( !m_zPending ) break;
        }

        // overlay gate
        if ( m_zState == ZState::Dead || m_zState == ZState::DoorEnter ||
            m_zState == ZState::Dance || m_zState == ZState::GameOver ) {
            c.mod.lockRunAxis = true; c.mod.runAxisMul = 0.f;
            if ( m_aState != AState::Neutral ) RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
        else if ( m_zState == ZState::Damaged ) {
            c.mod.lockRunAxis = true;
            if ( m_aState != AState::Neutral ) RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }

        // ===== Action =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_aPending ) { ApplyPendingAct ( c ); continue; }
            if ( m_aNeedEnter && m_action ) { m_action->OnEnter ( c ); m_aNeedEnter = false; }
            if ( m_action ) m_action->Update ( c , *this );
            if ( !m_aPending ) break;
        }

        // ===== Movement =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_mPending ) { ApplyPendingMove ( c ); continue; }
            if ( m_mNeedEnter && m_move ) { m_move->OnEnter ( c ); m_mNeedEnter = false; }
            if ( m_move ) m_move->Update ( c , *this );
            if ( !m_mPending ) break;
        }

        // debug snapshot
        m_dbg.mState = m_mState; m_dbg.aState = m_aState; m_dbg.zState = m_zState;
        m_dbg.groundedRaw = c.rep.grounded;
        m_dbg.groundedStable = c.rep.grounded || ( m_dbg.groundHoldT > 0.f );
        m_dbg.ignoreOneWay = c.ignoreOneWay;
        m_dbg.vx = c.vel.x; m_dbg.vy = c.vel.y;
        m_dbg.lastAABB = c.aabb; m_dbg.prevBottom = c.prevBottom;
        m_dbg.hp = m_health.hp; m_dbg.iFrameT = m_health.iFrameT;
        m_dbg.facing = m_facing; m_dbg.mouthFull = m_mouthFull; m_dbg.ability = m_ability;
        m_dbg.inhaleT = m_inhaleT; m_dbg.spitLockT = m_spitLockT;
        m_dbg.fallT = m_fallT; m_dbg.longFall = m_inLongFall;
    }

    void PlayerFSM::ResetForRespawn ( )
    {
        HardResetRuntime ( );
    }

    // damage / mouth / door / snapshot (unchanged except style)
    bool PlayerFSM::ApplyDamage ( const Damage& d )
    {
        return [ & ] {
            // 이미 사망/게임오버 상태면 추가 데미지 무시
            if ( m_zState == ZState::Dead || m_zState == ZState::GameOver )
                return false;

            bool applied = d.ignoreIFrames
                ? ( m_health.hp = std::max ( 0 , m_health.hp - std::max ( 0 , d.amount ) ) ,
                    m_health.iFrameT = m_cfg.iFrameMs ,
                    true )
                : m_health.Apply ( d.amount );

            if ( !applied )
                return false;

            auto kb = d.knockback;
            const float k = m_cfg.hurtKnockbackClamp;
            kb.x = std::clamp ( kb.x , -k , k );
            kb.y = std::clamp ( kb.y , -k , k );
            if ( d.additiveImpulse ) {
                auto v = m_body->Velocity ( );
                m_body->SetVelocity ( v + kb );
            }
            else {
                m_body->SetVelocity ( kb );
            }

            if ( m_mouthFull ) {
                m_mouthFull = false;
                m_caughtGift = Ability::None;
            }

            m_damagedT = m_cfg.damagedStun;

            if ( m_health.hp <= 0 ) {
                // 치명타 → 즉시 사망 오버레이
                RequestOver ( std::make_unique<Z_Dead> ( ) , ZState::Dead );
            }
            else {
                // 일반 피격 → Hurt 상태
                RequestOver ( std::make_unique<Z_Damaged> ( ) , ZState::Damaged );
            }

            return true;
            }( );
    }


    void PlayerFSM::OnMouthCatch ( Ability gift ) { m_mouthFull = true; m_caughtGift = gift; }

    PlayerFSM::Persistent PlayerFSM::SnapshotPersistent ( ) const {
        Persistent s{}; s.hp = m_health.hp; s.ability = m_ability; s.facing = m_facing; s.mouthFull = m_mouthFull; return s;
    }
    void PlayerFSM::RestorePersistent ( const Persistent& s ) {
        m_health.hp = std::clamp ( s.hp , 0 , m_cfg.maxHp );
        m_ability = s.ability;
        m_facing = ( s.facing >= 0 ) ? +1 : -1;
        m_mouthFull = false; m_caughtGift = Ability::None;
        m_dbg.hp = m_health.hp; m_dbg.ability = m_ability; m_dbg.facing = m_facing; m_dbg.mouthFull = m_mouthFull;
    }

    // ===== common systems =====
    void PlayerFSM::IntegrateAndCollide ( double fixedDt , const engine::Input& , Ctx& c )
    {
        // 1) 물리 통합 (DesiredRunAxis 는 movement state 가 세팅)
        c.body->AdvanceKinematics ( fixedDt );

        // 2) 코요테 / 점프 버퍼 → 자동 점프
        if ( c.body->Grounded ( ) ) m_dbg.coyoteT = m_cfg.coyoteMs;

        if ( ( c.body->Grounded ( ) || m_dbg.coyoteT > 0.f ) && m_dbg.bufferT > 0.f ) {
            c.body->Jump ( m_cfg.jumpSpeed );
            m_jumpLockT = m_cfg.jumpLockMs;
            m_dbg.coyoteT = 0.f; m_dbg.bufferT = 0.f;
            RequestMove ( std::make_unique<M_Jump> ( ) , MState::Jump );
        }

        // 3) 이번 프레임 예측 AABB
        float nx = 0.f , ny = 0.f;
        c.aabb = c.body->ProposeAABB ( fixedDt , &c.prevBottom , &nx , &ny );

        c.vel = c.body->Velocity ( );
        c.ignoreOneWay = ( m_dbg.dropT > 0.f ) || ( c.vel.y < 0.f );

        // 4) 월드 충돌 (solid / one-way / water 등)
        engine::physics::CollisionParams params{};
        c.col->MoveAndCollide ( c.aabb , c.vel , &c.rep , c.ignoreOneWay , c.prevBottom , params );
        c.body->ApplyCollisionResult ( c.aabb , c.vel , c.rep , nx , ny );

        // 5) 환경 플래그 업데이트 (CollisionReport 쪽에 inWater 가 있다고 가정)
        c.inWater = c.rep.inWater;
        c.waterGround = ( c.rep.inWater && c.rep.grounded );
        c.underwater = ( c.rep.inWater && !c.rep.grounded );
        m_dbg.inWater = c.inWater;

        // 6) 물 속에서는 전체 속도를 조금 줄여서 둔감하게
        if ( c.inWater ) {
            auto v = c.body->Velocity ( );
            const float drag = 0.5f; // 나중에 m_cfg.waterDrag 로 바꿔도 됨
            v.x *= drag;
            v.y *= drag;
            c.body->SetVelocity ( v );
        }

        // 7) 지상 관련 보조 / 롱폴 바운스 / 숏홉
        if ( c.rep.grounded ) m_dbg.groundHoldT = m_cfg.groundHoldMs;

        if ( c.rep.grounded && m_mState == MState::Fall && m_inLongFall && !m_bounceQueued ) {
            m_bounceQueued = true;
        }

        if ( !c.jumpHeld && c.body->Velocity ( ).y < 0.f && m_mState == MState::Jump ) {
            auto v = c.body->Velocity ( ); v.y *= m_cfg.shortHopMul; c.body->SetVelocity ( v );
        }
    }


    void PlayerFSM::UpdateFacing ( const Ctx& c )
    {
        if ( std::abs ( c.ax ) > 0.1f ) m_facing = ( c.ax >= 0.f ) ? +1 : -1;
        else { const auto v = c.body->Velocity ( ); if ( std::abs ( v.x ) > 1.f ) m_facing = ( v.x >= 0.f ) ? +1 : -1; }
    }

    engine::IntRect PlayerFSM::MakeInhaleBox ( const Ctx& c ) const
    {
        const auto& aabb = c.aabb;
        constexpr float W = 120.f , H = 80.f , yOff = -10.f;
        const int cx = ( aabb.l + aabb.r ) / 2;
        const int cy = ( aabb.t + aabb.b ) / 2 + static_cast< int >( yOff );

        engine::IntRect r{};
        if ( m_facing > 0 ) { r.l = cx; r.r = cx + static_cast< int >( W ); }
        else { r.l = cx - static_cast< int >( W ); r.r = cx; }
        r.t = cy - static_cast< int >( H * 0.5f );
        r.b = cy + static_cast< int >( H * 0.5f );
        return r;
    }

    void PlayerFSM::ResetFallAccumulators ( )
    {
        m_fallT = 0.f;
        m_tumbleT = 0.f;
        m_inLongFall = false;
        m_bounceQueued = false;
        m_fellFromJump = false;
    }

    // ===== transitions (same logic, formatted) =====
    void PlayerFSM::RequestMove ( std::unique_ptr<MBase> ns , MState tag ) { m_mPending = std::move ( ns ); m_mPendingTag = tag; }
    void PlayerFSM::RequestAct ( std::unique_ptr<ABase> ns , AState tag ) { m_aPending = std::move ( ns ); m_aPendingTag = tag; }
    void PlayerFSM::RequestOver ( std::unique_ptr<ZBase> ns , ZState tag ) { m_zPending = std::move ( ns ); m_zPendingTag = tag; }

    void PlayerFSM::ApplyPendingMove ( Ctx& )
    {
        if ( !m_mPending ) return;
        if ( !CanMove ( m_mState , m_mPendingTag , {} ) ) { m_mPending.reset ( ); return; }
        if ( m_aPendingTag == AState::SpitObject ) m_spitEmitted = false;
        if ( m_mState == MState::Fall && m_mPendingTag != MState::Fall ) ResetFallAccumulators ( );
        if ( m_move ) m_move->OnExit ( );
        m_move = std::move ( m_mPending );
        m_mState = m_mPendingTag;
        m_mNeedEnter = true;
    }
    void PlayerFSM::ApplyPendingAct ( Ctx& ) {
        if ( !m_aPending ) return;
        if ( !CanAct ( m_aState , m_aPendingTag , {} ) ) {
            m_aPending.reset ( ); return; 
        }
        if ( m_action ) m_action->OnExit ( );
        m_action = std::move ( m_aPending );
        m_aState = m_aPendingTag;
        m_aNeedEnter = true; 
    }

    void PlayerFSM::ApplyPendingOver ( Ctx& c )
    {
        if ( !m_zPending ) return;

        if ( !CanOver ( m_zState , m_zPendingTag , c ) ) {
            m_zPending.reset ( );
            return;
        }

        const ZState prev = m_zState;
        const ZState next = m_zPendingTag;

        if ( m_overlay ) m_overlay->OnExit ( );

        m_overlay = std::move ( m_zPending );
        m_zState = next;
        m_zNeedEnter = true;

        if ( prev != ZState::Dead && next == ZState::Dead ) {
            PlayerEvent ev{ PlayerEvent::Died };
            ev.rect = c.aabb;
            ev.facing = m_facing;
            m_events.push_back ( ev );
        }
    }


    bool PlayerFSM::CanMove ( MState from , MState to , const Ctx& ) const {
        if ( from == to ) return false;
        if ( m_zState == ZState::Dead || m_zState == ZState::GameOver ) return false;
        if ( from == MState::Jump && ( to == MState::Idle || to == MState::Walk || to == MState::Run ) && m_jumpLockT > 0.f ) return false;
        if ( from == MState::Jump && to == MState::Fall && m_body && m_body->Velocity ( ).y < 0.f ) return false;
        return true;
    }
    bool PlayerFSM::CanAct ( AState from , AState to , const Ctx& ) const {
        if ( from == to ) return false;
        if ( m_zState != ZState::None ) return false;
        if ( m_aState == AState::SpitObject && m_spitLockT > 0.f ) return false;
        return true;
    }
    bool PlayerFSM::CanOver ( ZState from , ZState to , const Ctx& ) const {
        if ( from == to ) return false;
        if ( from == ZState::Dead && to != ZState::GameOver ) return false;
        return true;
    }

    void PlayerFSM::HardResetRuntime ( )
    {
        m_health.Reset ( m_cfg.maxHp , m_cfg.iFrameMs );

        m_events.clear ( );
        m_mouthFull = false;
        m_caughtGift = Ability::None;
        m_ability = Ability::None;
        m_facing = +1;

        m_jumpLockT = 0.f;
        m_damagedT = 0.f;
        m_inhaleT = 0.f;
        m_spitLockT = 0.f;
        m_tapT = 0.f;
        m_slideT = 0.f;
        m_runQueued = false;
        m_lastTapDir = 0;
        m_pendingKB = { 0.f, 0.f };

        // fall/bounce
        m_fallT = 0.f;
        m_fallY0 = 0.f;
        m_tumbleT = 0.f;
        m_fellFromJump = false;
        m_inLongFall = false;
        m_bounceQueued = false;

        m_mPending.reset ( );
        m_aPending.reset ( );
        m_zPending.reset ( );
        m_transitionBudget = 0;

        m_move = std::make_unique<M_Idle> ( );
        m_mState = MState::Idle;
        m_mNeedEnter = true;

        m_action = std::make_unique<A_Neutral> ( );
        m_aState = AState::Neutral;
        m_aNeedEnter = true;

        m_overlay = std::make_unique<Z_None> ( );
        m_zState = ZState::None;
        m_zNeedEnter = true;

        if ( m_anim ) m_anim->Play ( "Idle" , true );

        m_dbg = {};
        m_dbg.hp = m_health.hp;
        m_dbg.iFrameT = m_health.iFrameT;
        m_dbg.mState = m_mState;
        m_dbg.aState = m_aState;
        m_dbg.zState = m_zState;
        m_dbg.facing = m_facing;
        m_dbg.mouthFull = m_mouthFull;
        m_dbg.ability = m_ability;
    }

} // namespace game
