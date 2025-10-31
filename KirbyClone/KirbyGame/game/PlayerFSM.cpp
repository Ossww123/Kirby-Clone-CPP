#include "game/PlayerFSM.h"
#include <cmath>
#include <algorithm>

using namespace engine;
using namespace engine::physics;

namespace game {

    // ====== 튜닝 상수 ======
    namespace {
        constexpr float TAP_WINDOW = 0.28f;  // 달리기 더블탭 허용 시간
        constexpr float RUN_TOGGLE_AX = 0.1f;  // 축 데드존
        constexpr float SLIDE_TIME = 0.22f;  // 슬라이딩킥 지속
        constexpr float SLIDE_VX = 280.f;  // 슬라이딩킥 초기 수평속도
        constexpr float FLAP_VY = -240.f; // 날개짓(Inflated 상태에서 Z) 상승속도
        constexpr float FLOAT_EXIT_VY = 60.f;  // Z 떼면 이 속도 이상 하강일 때 Inflated 종료
        inline float Abs ( float x ) { return x < 0.f ? -x : x; }
    }

    // ====== Public API ======
    void PlayerFSM::Init ( PhysicsBody* body ,
                         const CollisionSystem* worldCol ,
                         Animator* anim ,
                         const Cfg& cfg )
    {
        m_body = body; m_col = worldCol; m_anim = anim; m_cfg = cfg;

        m_health.Reset ( m_cfg.maxHp , m_cfg.iFrameMs );

        // --- 런타임 상태/타이머/이벤트 하드 리셋 ---
        m_events.clear ( );
        m_mouthFull = false;
        m_caughtGift = Ability::None;
        m_ability = Ability::None;   // ← 능력 초기화 핵심
        m_facing = +1;              // 원한다면 유지해도 됨
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
        m_fallT = 0.f; m_fallY0 = 0.f; m_tumbleT = 0.f;
        m_fellFromJump = false; m_inLongFall = false; m_bounceQueued = false;
        m_mPending.reset ( ); m_aPending.reset ( ); m_zPending.reset ( );
        m_transitionBudget = 0;

        m_move = std::make_unique<M_Idle> ( );    m_mState = MState::Idle;    m_mNeedEnter = true;
        m_action = std::make_unique<A_Neutral> ( ); m_aState = AState::Neutral; m_aNeedEnter = true;
        m_overlay = std::make_unique<Z_None> ( );    m_zState = ZState::None;    m_zNeedEnter = true;

        if ( m_anim ) m_anim->Play ( "Idle" , true );

        m_dbg = {};
        m_dbg.hp = m_health.hp; m_dbg.iFrameT = m_health.iFrameT;
        m_dbg.mState = m_mState; m_dbg.aState = m_aState; m_dbg.zState = m_zState;
        m_dbg.facing = m_facing; m_dbg.mouthFull = m_mouthFull; m_dbg.ability = m_ability;
    }

    void PlayerFSM::Step ( double fixedDt , const Input& input )
    {
        if ( !m_body || !m_col ) return;

        Ctx c;
        c.body = m_body; c.col = m_col; c.anim = m_anim; c.cfg = m_cfg;
        c.dt = static_cast< float >( fixedDt );

        // --- 입력 ---
        c.ax = input.GetAxis ( "MoveX" );
        c.ay = input.GetAxis ( "MoveY" );
        c.jumpPressed = input.ActionPressed ( "Jump" );   // Z
        c.jumpHeld = input.ActionDown ( "Jump" );
        c.attackPressed = input.ActionPressed ( "Attack" );                                 // X (탭)
        c.attackHeld = input.ActionDown ( "Attack" );                                    // X (홀드)
        c.abilityPressed = input.ActionPressed ( "Ability" );                                // 사용 안 함(선택)
        c.interactPressed = input.ActionPressed ( "Interact" );                             // 위(문 입장 신호)

        // 방향키 '탭' 이벤트(달리기 더블탭 감지)
        const bool leftPressed = input.Pressed ( VK_LEFT );
        const bool rightPressed = input.Pressed ( VK_RIGHT );
        m_tapT = std::max ( 0.f , m_tapT - c.dt );
        auto onTap = [ & ] ( int dir , bool pressed ) {
            if ( !pressed ) return;
            // 지상에서 같은 방향을 TAP_WINDOW 내에 두 번 누르면 Run 예약
            if ( m_lastTapDir == dir && m_tapT > 0.f &&
                ( m_body->Grounded ( ) || m_dbg.groundHoldT > 0.f ) )
                m_runQueued = true;
            m_lastTapDir = dir;
            m_tapT = TAP_WINDOW;
            };
        onTap ( -1 , leftPressed );
        onTap ( +1 , rightPressed );

        // --- 타이머/체력 ---
        if ( c.jumpPressed ) m_dbg.bufferT = m_cfg.bufferMs;
        else               m_dbg.bufferT = std::max ( 0.f , m_dbg.bufferT - c.dt );
        m_dbg.coyoteT = std::max ( 0.f , m_dbg.coyoteT - c.dt );
        m_dbg.dropT = std::max ( 0.f , m_dbg.dropT - c.dt );
        m_dbg.groundHoldT = std::max ( 0.f , m_dbg.groundHoldT - c.dt );
        m_jumpLockT = std::max ( 0.f , m_jumpLockT - c.dt );
        m_spitLockT = std::max ( 0.f , m_spitLockT - c.dt );
        m_slideT = std::max ( 0.f , m_slideT - c.dt );
        m_health.Tick ( c.dt );

        // 드롭스루: 지상 + 아래 + 점프
        if ( m_body->Grounded ( ) && c.ay < -0.5f && c.jumpPressed )
            m_dbg.dropT = m_cfg.dropMs;

        // 물리/충돌 1회
        IntegrateAndCollide ( fixedDt , input , c );

        // 바라보는 방향
        UpdateFacing ( c );

        // ===== Overlay (최우선) =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_zPending ) { ApplyPendingOver ( c ); continue; }
            if ( m_zNeedEnter && m_overlay ) { m_overlay->OnEnter ( c ); m_zNeedEnter = false; }
            if ( m_overlay ) m_overlay->Update ( c , *this );
            if ( !m_zPending ) break;
        }

        // Overlay 게이트
        if ( m_zState == ZState::Dead || m_zState == ZState::DoorEnter ||
            m_zState == ZState::Dance || m_zState == ZState::GameOver ) {
            c.mod.lockRunAxis = true; c.mod.runAxisMul = 0.f;
            if ( m_aState != AState::Neutral ) RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
        else if ( m_zState == ZState::Damaged ) {
            c.mod.lockRunAxis = true;
            if ( m_aState != AState::Neutral ) RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }

        // ===== Action (이벤트/모디파이어 생산) =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_aPending ) { ApplyPendingAct ( c ); continue; }
            if ( m_aNeedEnter && m_action ) { m_action->OnEnter ( c ); m_aNeedEnter = false; }
            if ( m_action ) m_action->Update ( c , *this );
            if ( !m_aPending ) break;
        }

        // ===== Movement (모디파이어 소비) =====
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {
            if ( m_mPending ) { ApplyPendingMove ( c ); continue; }
            if ( m_mNeedEnter && m_move ) { m_move->OnEnter ( c ); m_mNeedEnter = false; }
            if ( m_move ) m_move->Update ( c , *this );
            if ( !m_mPending ) break;
        }

        // 디버그 스냅샷
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

    bool PlayerFSM::ApplyDamage ( const Damage& d )
    {
        if ( m_zState == ZState::Dead || m_zState == ZState::GameOver ) return false;

        bool applied = d.ignoreIFrames
            ? ( m_health.hp = std::max ( 0 , m_health.hp - std::max ( 0 , d.amount ) ) , m_health.iFrameT = m_cfg.iFrameMs , true )
            : m_health.Apply ( d.amount );

        if ( !applied ) return false;

        auto kb = d.knockback;
        const float k = m_cfg.hurtKnockbackClamp;
        kb.x = std::clamp ( kb.x , -k , k );
        kb.y = std::clamp ( kb.y , -k , k );
        if ( d.additiveImpulse ) { auto v = m_body->Velocity ( ); m_body->SetVelocity ( v + kb ); }
        else { m_body->SetVelocity ( kb ); }

        // 머금기/능력 정리
        if ( m_mouthFull ) { m_mouthFull = false; m_caughtGift = Ability::None; }
        // (원하면 여기서 능력 드랍 이벤트 생성)
        // if (m_ability!=Ability::None) { ...; m_ability=Ability::None; }

        // 경직
        m_damagedT = m_cfg.damagedStun;
        RequestOver ( std::make_unique<Z_Damaged> ( ) , ZState::Damaged );
        return true;
    }

    void PlayerFSM::OnMouthCatch ( Ability gift )
    {
        m_mouthFull = true;
        m_caughtGift = gift;
    }

    void PlayerFSM::BeginDoorEnter ( ) {
        RequestOver ( std::make_unique<Z_DoorEnter> ( ) , ZState::DoorEnter );        
    }

    void PlayerFSM::EndDoorEnter ( ) {
        RequestOver ( std::make_unique<Z_None> ( ) , ZState::None );        
    }

    PlayerFSM::Persistent PlayerFSM::SnapshotPersistent ( ) const
    {
        Persistent s{};
        s.hp = m_health.hp;
        s.ability = m_ability;
        s.facing = m_facing;
        s.mouthFull = m_mouthFull;
        return s;
    }

    void PlayerFSM::RestorePersistent ( const Persistent& s )
    {
        // 능력/HP/방향 복원 (경계/일관성 정리)
        m_health.hp = std::clamp ( s.hp , 0 , m_cfg.maxHp );
        m_ability = s.ability;
        m_facing = ( s.facing >= 0 ) ? +1 : -1;
        // 문 이동에서는 입에 머금은 상태는 비우는 편이 일반적
        m_mouthFull = false;
        m_caughtGift = Ability::None;
        // 디버그 스냅샷도 맞춰줌(다음 Step 전 HUD 안정)
        m_dbg.hp = m_health.hp;
        m_dbg.ability = m_ability;
        m_dbg.facing = m_facing;
        m_dbg.mouthFull = m_mouthFull;
    }


    // ====== 공통 시스템 ======
    void PlayerFSM::IntegrateAndCollide ( double fixedDt , const Input& , Ctx& c )
    {
        c.body->AdvanceKinematics ( fixedDt );

        if ( c.body->Grounded ( ) ) m_dbg.coyoteT = m_cfg.coyoteMs;

        if ( ( c.body->Grounded ( ) || m_dbg.coyoteT > 0.f ) && m_dbg.bufferT > 0.f ) {
            c.body->Jump ( m_cfg.jumpSpeed );
            m_jumpLockT = m_cfg.jumpLockMs;
            m_dbg.coyoteT = 0.f; m_dbg.bufferT = 0.f;
            RequestMove ( std::make_unique<M_Jump> ( ) , MState::Jump );
        }

        float nx = 0.f , ny = 0.f;
        c.aabb = c.body->ProposeAABB ( fixedDt , &c.prevBottom , &nx , &ny );
        c.vel = c.body->Velocity ( );
        c.ignoreOneWay = ( m_dbg.dropT > 0.f ) || ( c.vel.y < 0.f );
        c.col->MoveAndCollide ( c.aabb , c.vel , &c.rep , c.ignoreOneWay , c.prevBottom );
        c.body->ApplyCollisionResult ( c.aabb , c.vel , c.rep , nx , ny );

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
        if ( Abs ( c.ax ) > 0.1f ) m_facing = ( c.ax >= 0.f ) ? +1 : -1;
        else { const auto v = c.body->Velocity ( ); if ( Abs ( v.x ) > 1.f ) m_facing = ( v.x >= 0.f ) ? +1 : -1; }
    }

    RECT PlayerFSM::MakeInhaleBox ( const Ctx& c ) const
    {
        RECT aabb = c.aabb; const float W = 120.f , H = 80.f , yOff = -10.f;
        const int cx = ( aabb.left + aabb.right ) / 2;
        const int cy = ( aabb.top + aabb.bottom ) / 2 + static_cast< int >( yOff );
        RECT r{}; if ( m_facing > 0 ) { r.left = cx; r.right = cx + ( int ) W; }
        else { r.left = cx - ( int ) W; r.right = cx; }
        r.top = cy - ( int ) ( H * 0.5f ); r.bottom = cy + ( int ) ( H * 0.5f ); return r;
    }

    // --- Reset accumulated fall value ---
    void PlayerFSM::ResetFallAccumulators ( )
    {
        m_fallT = 0.f;
        m_tumbleT = 0.f;
        m_inLongFall = false;
        m_bounceQueued = false;
        m_fellFromJump = false;
        // m_fallY0는 새로 Fall에 들어가는 1프레임 차에 다시 설정됨(M_Fall::Update 첫 분기)
    }

    // ====== Movement ======
    void PlayerFSM::M_Grounded::Update ( Ctx& c , PlayerFSM& f )
    {
        float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // 공통: 지면 이탈 감시
        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( !stable ) {
            f.m_fallT = 0.f;
            f.m_fellFromJump = false;                  // 지면 이탈로 인한 낙하
            f.m_tumbleT = 0.f;
            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            return;
        }
        // 웅크리기: 아래 입력이면 Crouch으로
        if ( c.ay < -0.5f ) {
            if ( f.m_mState != MState::Crouch && f.m_mState != MState::Slide )
                f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch );
        }
    }

    void PlayerFSM::M_Airborne::Update ( Ctx& c , PlayerFSM& f )
    {
        float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // 공중에서 Z(점프) 눌러 공기 머금기 시작
        if ( ( f.m_mState == MState::Jump || f.m_mState == MState::Fall ) && c.jumpPressed ) {
            f.RequestMove ( std::make_unique<M_Inflated> ( ) , MState::Inflated );
        }

        const bool stable = ( c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f ) ) && ( c.vel.y >= 0.f );
        if ( stable ) {
            if ( f.m_bounceQueued ) {
                auto v = c.body->Velocity ( );
                v.y = -std::abs ( f.m_cfg.bounceSpeedUp );
                c.body->SetVelocity ( v );
                f.m_bounceQueued = false;
                f.m_inLongFall = false;
                f.m_fallT = 0.f; f.m_tumbleT = 0.f;
                Play ( c.anim , "Bounce" , true );
                return; // 이번 틱에는 지상 전이 금지, 공중 유지
            }
            if ( Abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
            else                                f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    void PlayerFSM::M_Idle::OnEnter ( Ctx& c ) { Play ( c.anim , "Idle" , true ); }
    void PlayerFSM::M_Idle::Update ( Ctx& c , PlayerFSM& f )
    {
        c.body->SetDesiredRunAxis ( 0.f );
        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( !stable ) { f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall ); return; }

        if ( c.ay < -0.5f ) { f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch ); return; }
        if ( Abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
    }

    void PlayerFSM::M_Walk::OnEnter ( Ctx& c ) { Play ( c.anim , "Walk" , true ); }
    void PlayerFSM::M_Walk::Update ( Ctx& c , PlayerFSM& f )
    {
        M_Grounded::Update ( c , f );
        if ( f.m_mState != MState::Walk ) return; // 위에서 전이됐으면 조기 종료

        if ( Abs ( c.ax ) <= RUN_TOGGLE_AX ) { f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle ); return; }

        // 더블탭으로 Run 전이
        if ( f.m_runQueued ) {
            f.m_runQueued = false;
            f.RequestMove ( std::make_unique<M_Run> ( ) , MState::Run );
            return;
        }
    }

    void PlayerFSM::M_Run::OnEnter ( Ctx& c ) { Play ( c.anim , "Run" , true ); }
    void PlayerFSM::M_Run::Update ( Ctx& c , PlayerFSM& f )
    {
        M_Grounded::Update ( c , f );
        if ( f.m_mState != MState::Run ) return;

        // 입력 약화/해제 시 Walk로
        if ( Abs ( c.ax ) < 0.75f ) { f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk ); return; }
        // 아래 입력이면 바로 Crouch(원작 느낌에 맞게 Run->슬라이드 진입은 Crouch 후 Z/X)
        if ( c.ay < -0.5f ) { f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch ); return; }
    }

    void PlayerFSM::M_Crouch::OnEnter ( Ctx& c ) { Play ( c.anim , "Crouch" , true ); }
    void PlayerFSM::M_Crouch::Update ( Ctx& c , PlayerFSM& f )
    {
        c.body->SetDesiredRunAxis ( 0.f );

        // Z 또는 X 누르면 슬라이딩킥
        if ( c.jumpPressed || c.attackPressed ) {
            f.m_slideT = SLIDE_TIME;
            // 초기 수평속도 부여
            auto v = c.body->Velocity ( ); v.x = ( float ) f.m_facing * SLIDE_VX;
            c.body->SetVelocity ( v );
            f.RequestMove ( std::make_unique<M_Slide> ( ) , MState::Slide );
            return;
        }

        // 아래 떼면 Idle 복귀
        if ( c.ay >= -0.5f ) { f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle ); }
    }

    void PlayerFSM::M_Slide::OnEnter ( Ctx& c ) { Play ( c.anim , "Slide" , true ); }
    void PlayerFSM::M_Slide::Update ( Ctx& c , PlayerFSM& f )
    {
        // 슬라이드 중에는 입력 무시, 관성 유지
        c.body->SetDesiredRunAxis ( ( float ) f.m_facing );

        if ( f.m_slideT <= 0.f ) {
            // 종료: 아래 계속이면 Crouch, 아니면 Idle
            if ( c.ay < -0.5f ) f.RequestMove ( std::make_unique<M_Crouch> ( ) , MState::Crouch );
            else               f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    void PlayerFSM::M_Jump::OnEnter ( Ctx& c ) { Play ( c.anim , "Jump" , true ); }
    void PlayerFSM::M_Jump::Update ( Ctx& c , PlayerFSM& f )
    {
        float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        if ( f.m_jumpLockT > 0.f ) return;

        // 공중에서 Z 다시 누르면 Inflated
        if ( c.jumpPressed ) { f.RequestMove ( std::make_unique<M_Inflated> ( ) , MState::Inflated ); return; }

        if ( f.m_jumpLockT > 0.f ) return;
        if ( c.body->Velocity ( ).y >= 0.f ) {
            f.m_fallT = 0.f;
            f.m_fellFromJump = true;
            f.m_tumbleT = f.m_cfg.fallTumbleMs;
            f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            return;
        }

        const bool stable = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
        if ( stable ) {
            if ( Abs ( c.ax ) > RUN_TOGGLE_AX ) f.RequestMove ( std::make_unique<M_Walk> ( ) , MState::Walk );
            else                           f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
        }
    }

    void PlayerFSM::M_Fall::OnEnter ( Ctx & c ) {}
    void PlayerFSM::M_Fall::Update ( Ctx & c , PlayerFSM & f )
    {
        // 좌우 이동
        float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // 진입 1프레임: FALL0 또는 FALL1 시작
        if ( f.m_fallT <= 0.f ) {
            Play ( c.anim , ( f.m_tumbleT > 0.f ? "Fall0" : "Fall1" ) , true );
            f.m_inLongFall = false;
            f.m_bounceQueued = false;
            f.m_fallY0 = static_cast< float >( c.aabb.bottom );
        }

        // 페이즈 타이머
        f.m_fallT += c.dt;
        if ( f.m_tumbleT > 0.f ) {
            f.m_tumbleT = std::max ( 0.f , f.m_tumbleT - c.dt );
            if ( f.m_tumbleT <= 0.f ) Play ( c.anim , "Fall1" , true );
        }
        // 장낙하 진입 조건: 시간 OR 누적높이
        if ( !f.m_inLongFall ) {
            const float dropPx = static_cast< float >( c.aabb.bottom ) - f.m_fallY0;
            if ( f.m_fallT >= f.m_cfg.fallLongMs || dropPx >= f.m_cfg.fallLongHeightPx ) {
                f.m_inLongFall = true;
                Play ( c.anim , "Fall2" , true );
            }
        }
        // 일반 공중 공통 처리(착지/전이/바운스)는 베이스로 위임
        M_Airborne::Update ( c , f );
    }

    void PlayerFSM::M_Inflated::OnEnter ( Ctx& c ) { Play ( c.anim , "Inflate" , true ); }
    void PlayerFSM::M_Inflated::Update ( Ctx& c , PlayerFSM& f )
    {
        // 좌우 이동은 입력대로(원하면 감속을 주고 싶으면 runAxisMul 조정)
        float axis = c.ax * ( c.mod.lockRunAxis ? 0.f : c.mod.runAxisMul );
        c.body->SetDesiredRunAxis ( axis );

        // 날개짓(Z 탭): 즉시 약간 상승
        if ( c.jumpPressed ) {
            auto v = c.body->Velocity ( );
            v.y = -240.f;                    // 필요 시 튜닝
            if ( v.y > -240.f ) v.y = -240.f;  // 최소 상승 보장
            c.body->SetVelocity ( v );
        }

        // 소프트 폴: 낙하 속도 상한(천천히 내려오게)
        auto v = c.body->Velocity ( );
        const float MAX_FALL_VY = 80.f;      // 필요 시 튜닝
        if ( v.y > MAX_FALL_VY ) { v.y = MAX_FALL_VY; c.body->SetVelocity ( v ); }
    }

    // ====== Action ======
    void PlayerFSM::A_Neutral::Update ( Ctx& c , PlayerFSM& f )
    {
        // 우선순위: (1) 머금은 물체 뱉기 (2) 공기포 (Inflated) (3) 카피능력 공격 (4) 빨아들이기
        if ( f.m_mouthFull && c.attackPressed ) { f.RequestAct ( std::make_unique<A_SpitObject> ( ) , AState::SpitObject ); return; }
        if ( f.m_mState == MState::Inflated && c.attackPressed ) { f.RequestAct ( std::make_unique<A_AirPuff> ( ) , AState::AirPuff ); return; }
        if ( f.m_ability != Ability::None && c.attackPressed ) {
            // Crouch/Slide 중에 능력공격을 금지 가드를 추가
            f.RequestAct ( std::make_unique<A_AbilityAtk> ( ) , AState::AbilityAtk ); return;
        }
        // Inhale 가드: Crouch/Slide 중이거나 아래 입력 중일 때는 Inhale 금지 (슬라이딩킥 우선)
        const bool crouchLike = ( f.m_mState == MState::Crouch || f.m_mState == MState::Slide || c.ay < -0.5f );
        if ( !f.m_mouthFull && c.attackHeld && f.m_ability == Ability::None && !crouchLike ) {
            f.RequestAct ( std::make_unique<A_Inhale> ( ) , AState::Inhale ); return;
        }
    }

    void PlayerFSM::A_Inhale::OnEnter ( Ctx& c ) { Play ( c.anim , "Inhale" , true ); }
    void PlayerFSM::A_Inhale::Update ( Ctx& c , PlayerFSM& f )
    {
        // Crouch/Slide에 들어가면 Inhale 즉시 취소(동시 발동 방지)
        if ( f.m_mState == MState::Crouch || f.m_mState == MState::Slide ) {
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
            return;
        }

        c.mod.runAxisMul = 0.5f;
        if ( f.m_inhaleT <= 0.f ) f.m_inhaleT = 0.55f;
        f.m_inhaleT = std::max ( 0.f , f.m_inhaleT - c.dt );

        PlayerEvent ev{ PlayerEvent::InhaleVolume }; ev.rect = f.MakeInhaleBox ( c ); ev.facing = f.m_facing; f.m_events.push_back ( ev );

        if ( f.m_mouthFull ) { f.RequestAct ( std::make_unique<A_MouthFull> ( ) , AState::MouthFull ); return; }
        if ( !c.attackHeld || f.m_inhaleT <= 0.f ) { f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral ); }
    }

    void PlayerFSM::A_MouthFull::OnEnter ( Ctx& c ) { Play ( c.anim , "MouthFullIdle" , true ); }
    void PlayerFSM::A_MouthFull::Update ( Ctx& c , PlayerFSM& f )
    {
        if ( !f.m_mouthFull ) { f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral ); return; }

        // X: 뱉기
        if ( c.attackPressed ) { f.RequestAct ( std::make_unique<A_SpitObject> ( ) , AState::SpitObject ); return; }

        // 아래키로 삼키기(카피)
        if ( c.ay < -0.5f ) {
            PlayerEvent e1{ PlayerEvent::SwallowAbility }; e1.ability = f.m_caughtGift; f.m_events.push_back ( e1 );
            f.m_ability = f.m_caughtGift; f.m_caughtGift = Ability::None;
            f.m_mouthFull = false;
            PlayerEvent e2{ PlayerEvent::AbilityGained }; e2.ability = f.m_ability; f.m_events.push_back ( e2 );
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
            return;
        }
    }

    void PlayerFSM::A_SpitObject::OnEnter ( Ctx& c ) { Play ( c.anim , "Spit" , true ); }
    void PlayerFSM::A_SpitObject::Update ( Ctx& c , PlayerFSM& f )
    {
        // 최초 1프레임에만 발사
        if ( !f.m_spitEmitted ) {
            PlayerEvent ev{ PlayerEvent::SpitStar };
            ev.facing = f.m_facing;
            f.m_events.push_back ( ev );
            f.m_spitLockT = 0.18f;                 // 짧은 공격 락
            f.m_mouthFull = false;
            f.m_caughtGift = Ability::None;
            f.m_spitEmitted = true;                // 원샷 가드
        }
        // 락 동안 입력 잠금
        c.mod.lockRunAxis = true;
        // 락이 끝나면 Neutral로 복귀 (재발사 없음)
        if ( f.m_spitEmitted && f.m_spitLockT <= 0.f ) {
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
        }
    }

    void PlayerFSM::A_AirPuff::OnEnter ( Ctx& c ) { Play ( c.anim , "AirPuff" , true ); }
    void PlayerFSM::A_AirPuff::Update ( Ctx& c , PlayerFSM& f )
    {
        // 최초 프레임에 공기포 1회 발사
        if ( f.m_spitLockT <= 0.f ) {
            PlayerEvent ev{ PlayerEvent::AirPuffShot }; ev.facing = f.m_facing; f.m_events.push_back ( ev );
            f.m_spitLockT = 0.14f;

            // 공기포를 쏘면 바로 Inflated 해제(움직임은 상황에 맞게)
            const bool grounded = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
            if ( f.m_mState == MState::Inflated ) {
                if ( grounded ) f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
                else          f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
            }
        }

        // 짧은 발사 락
        c.mod.lockRunAxis = true;
        f.m_spitLockT = std::max ( 0.f , f.m_spitLockT - c.dt );
        if ( f.m_spitLockT <= 0.f ) f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
    }

    void PlayerFSM::A_AbilityAtk::OnEnter ( Ctx& c ) { Play ( c.anim , "AbilityAtk" , true ); }
    void PlayerFSM::A_AbilityAtk::Update ( Ctx& c , PlayerFSM& f )
    {
        // 최초 프레임에 능력별 공격 1회 발행
        if ( f.m_spitLockT <= 0.f ) {
            switch ( f.m_ability ) {
            case Ability::Fire: {
                PlayerEvent ev{ PlayerEvent::AbilityFire };
                ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.22f;     // 불꽃 짧은 유지
                break;
            }
            case Ability::Spark: {
                PlayerEvent ev{ PlayerEvent::AbilitySpark };
                ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.25f;     // 스파크 링
                break;
            }
            case Ability::Beam: {
                PlayerEvent ev{ PlayerEvent::AbilityBeam };
                ev.facing = f.m_facing; ev.ability = f.m_ability;
                f.m_events.push_back ( ev );
                f.m_spitLockT = 0.18f;     // 빔 채찍
                break;
            }
            default:
                // 능력 없으면 바로 종료
                f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
                return;
            }
        }

        // 공격 중 입력 잠금(히트확정 연출)
        c.mod.lockRunAxis = true;

        if ( !c.attackHeld && f.m_spitLockT > 0.f ) {
            f.m_spitLockT = 0.f;
        }

        // 락 해제되면 Neutral 복귀
        f.m_spitLockT = std::max ( 0.f , f.m_spitLockT - c.dt );
        if ( f.m_spitLockT <= 0.f )
            f.RequestAct ( std::make_unique<A_Neutral> ( ) , AState::Neutral );
    }

    // ====== Overlay ======
    void PlayerFSM::Z_None::Update ( Ctx& c , PlayerFSM& f )
    {
        // Overlay 제약이 없을 때만 Interact 처리(피격/사망/연출 중엔 차단)
        if ( c.interactPressed ) {
            PlayerEvent ev{ PlayerEvent::DoorInteract };
            ev.rect = c.aabb;        // 현재 커비 AABB (원하면 사용)
            ev.facing = f.m_facing;
            f.m_events.push_back ( ev );
        }
    }

    void PlayerFSM::Z_Damaged::OnEnter ( Ctx& c ) { Play ( c.anim , "Hurt" , true ); }
    void PlayerFSM::Z_Damaged::Update ( Ctx& c , PlayerFSM& f )
    {
        c.mod.lockRunAxis = true;

        // 데미지 들어오면 공기머금기 해제
        if ( f.m_mState == MState::Inflated ) {
            const bool grounded = c.body->Grounded ( ) || ( f.m_dbg.groundHoldT > 0.f );
            if ( grounded ) f.RequestMove ( std::make_unique<M_Idle> ( ) , MState::Idle );
            else          f.RequestMove ( std::make_unique<M_Fall> ( ) , MState::Fall );
        }

        f.m_damagedT = std::max ( 0.f , f.m_damagedT - c.dt );
        if ( f.m_damagedT <= 0.f ) f.RequestOver ( std::make_unique<Z_None> ( ) , ZState::None );
    }

    void PlayerFSM::Z_Dead::OnEnter ( Ctx& c ) { Play ( c.anim , "Death" , true ); }
    void PlayerFSM::Z_Dead::Update ( Ctx& c , PlayerFSM& ) { c.mod.lockRunAxis = true; }

    void PlayerFSM::Z_DoorEnter::OnEnter ( Ctx& c ) { Play ( c.anim , "DoorEnter" , true ); }
    void PlayerFSM::Z_DoorEnter::Update ( Ctx& c , PlayerFSM& ) {}

    void PlayerFSM::Z_Dance::OnEnter ( Ctx& c ) { Play ( c.anim , "Dance" , true ); }
    void PlayerFSM::Z_Dance::Update ( Ctx& c , PlayerFSM& ) {}

    void PlayerFSM::Z_GameOver::OnEnter ( Ctx& c ) { Play ( c.anim , "GameOver" , true ); }
    void PlayerFSM::Z_GameOver::Update ( Ctx& c , PlayerFSM& ) {}

    // ====== 전이 & 가드 ======
    void PlayerFSM::RequestMove ( std::unique_ptr<MBase> ns , MState tag ) { m_mPending = std::move ( ns ); m_mPendingTag = tag; }
    void PlayerFSM::RequestAct ( std::unique_ptr<ABase> ns , AState tag ) { m_aPending = std::move ( ns ); m_aPendingTag = tag; }
    void PlayerFSM::RequestOver ( std::unique_ptr<ZBase> ns , ZState tag ) { m_zPending = std::move ( ns ); m_zPendingTag = tag; }

    void PlayerFSM::ApplyPendingMove ( Ctx& )
    {
        if ( !m_mPending ) return;
        if ( !CanMove ( m_mState , m_mPendingTag , {} ) ) { m_mPending.reset ( ); return; }
        // SpitObject 진입 시 one-shot 플래그 리셋
        if ( m_aPendingTag == AState::SpitObject ) {
            m_spitEmitted = false;
        }
        // Fall → (다른 상태) 로 나갈 때, 장낙하/높이 누적 리셋
        if ( m_mState == MState::Fall && m_mPendingTag != MState::Fall ) {
            ResetFallAccumulators ( );
        }
        if ( m_move ) m_move->OnExit ( );
        m_move = std::move ( m_mPending );
        m_mState = m_mPendingTag;
        m_mNeedEnter = true;
    }
    void PlayerFSM::ApplyPendingAct ( Ctx& ) { if ( !m_aPending ) return; if ( !CanAct ( m_aState , m_aPendingTag , {} ) ) { m_aPending.reset ( ); return; } if ( m_action ) m_action->OnExit ( ); m_action = std::move ( m_aPending ); m_aState = m_aPendingTag; m_aNeedEnter = true; }
    void PlayerFSM::ApplyPendingOver ( Ctx& ) { if ( !m_zPending ) return; if ( !CanOver ( m_zState , m_zPendingTag , {} ) ) { m_zPending.reset ( ); return; } if ( m_overlay ) m_overlay->OnExit ( ); m_overlay = std::move ( m_zPending ); m_zState = m_zPendingTag; m_zNeedEnter = true; }

    bool PlayerFSM::CanMove ( MState from , MState to , const Ctx& c ) const
    {
        if ( from == to ) return false;
        if ( m_zState == ZState::Dead || m_zState == ZState::GameOver ) return false;
        if ( from == MState::Jump && ( to == MState::Idle || to == MState::Walk || to == MState::Run ) && m_jumpLockT > 0.f ) return false;
        if ( from == MState::Jump && to == MState::Fall && m_body && m_body->Velocity ( ).y < 0.f ) return false;
        return true;
    }
    bool PlayerFSM::CanAct ( AState from , AState to , const Ctx& ) const
    {
        if ( from == to ) return false;
        if ( m_zState != ZState::None ) return false;
        if ( m_aState == AState::SpitObject && m_spitLockT > 0.f ) return false;
        return true;
    }
    bool PlayerFSM::CanOver ( ZState from , ZState to , const Ctx& ) const
    {
        if ( from == to ) return false;
        if ( from == ZState::Dead && to != ZState::GameOver ) return false;
        return true;
    }

} // namespace game
