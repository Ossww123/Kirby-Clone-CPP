#include "game/PlayerFSM.h"
#include <algorithm>
#include <cmath>
using namespace engine;
using namespace engine::physics;

namespace game {

    // ====== 외부 인터페이스 ======
    void PlayerFSM::Init ( PhysicsBody* body ,
                         const CollisionSystem* worldCol ,
                         Animator* anim ,
                         const Cfg& cfg )
    {
        m_body = body; m_col = worldCol; m_anim = anim; m_cfg = cfg;

        // 초기 상태를 즉시 세팅
        m_cur = std::make_unique<Idle> ( );
        m_state = PState::Idle;
        m_needEnter = true;

        if ( m_anim ) m_anim->Play ( "Idle" , true );
    }

    void PlayerFSM::Step ( double fixedDt , const Input& input )
    {
        if ( !m_body || !m_col ) return;

        Ctx c;
        c.body = m_body; c.col = m_col; c.anim = m_anim; c.cfg = m_cfg;
        c.dt = static_cast< float >( fixedDt );

        // ---- 입력/타이머 ----
        c.ax = input.GetAxis ( "MoveX" );
        c.ay = input.GetAxis ( "MoveY" );
        c.jumpPressed = input.ActionPressed ( "Jump" ) || input.Pressed ( VK_SPACE );
        c.jumpHeld = input.ActionDown ( "Jump" ) || input.Down ( VK_SPACE );

        // 타이머 갱신
        if ( c.jumpPressed ) m_dbg.bufferT = m_cfg.bufferMs;
        else               m_dbg.bufferT = std::max ( 0.f , m_dbg.bufferT - c.dt );

        m_dbg.coyoteT = std::max ( 0.f , m_dbg.coyoteT - c.dt );
        m_dbg.dropT = std::max ( 0.f , m_dbg.dropT - c.dt );
        m_dbg.groundHoldT = std::max ( 0.f , m_dbg.groundHoldT - c.dt );
        m_jumpLockT = std::max ( 0.f , m_jumpLockT - c.dt );

        // 드롭스루 시작: 지상 + 아래 + 점프
        if ( m_body->Grounded ( ) && c.ay < -0.5f && c.jumpPressed ) m_dbg.dropT = m_cfg.dropMs;

        // ---- 물리/충돌 (한 번만) ----
        IntegrateAndCollide ( fixedDt , input , c );

        // ---- 전이 처리 루프(중앙) ----
        m_transitionBudget = std::max ( 1 , m_cfg.maxTransitionsPerStep );
        while ( m_transitionBudget-- > 0 ) {

            // 1) 보류 전이가 있으면 먼저 적용 (이전 상태 Update가 이를 되돌리는 걸 방지)
            if ( m_pending ) { ApplyPending ( c ); continue; }

            // 2) 새 상태 첫 진입 훅
            if ( m_needEnter && m_cur ) { m_cur->OnEnter ( c ); m_needEnter = false; }

            // 3) 상태 업데이트(여기서 RequestChange 가능)
            if ( m_cur ) m_cur->Update ( c , *this );

            // 4) 더 이상 보류 전이가 없다면 루프 종료
            if ( !m_pending ) break;
        }

        // ---- 디버그 스냅샷 ----
        m_dbg.state = m_state;
        m_dbg.groundedRaw = c.rep.grounded;
        m_dbg.groundedStable = c.rep.grounded || ( m_dbg.groundHoldT > 0.f );
        m_dbg.ignoreOneWay = c.ignoreOneWay;
        m_dbg.vx = c.vel.x;
        m_dbg.vy = c.vel.y;
        m_dbg.lastAABB = c.aabb;
        m_dbg.prevBottom = c.prevBottom;
    }

    // ====== 공통 시스템 스텝 ======
    void PlayerFSM::IntegrateAndCollide ( double fixedDt , const Input& , Ctx& c )
    {
        // 가속/중력 적분
        c.body->AdvanceKinematics ( fixedDt );

        // 코요테: 지상일 때 리필
        if ( c.body->Grounded ( ) ) m_dbg.coyoteT = m_cfg.coyoteMs;

        // 버퍼 점프 처리(지상/코요테 중)
        if ( ( c.body->Grounded ( ) || m_dbg.coyoteT > 0.f ) && m_dbg.bufferT > 0.f ) {
            c.body->Jump ( m_cfg.jumpSpeed );
            m_jumpLockT = m_cfg.jumpLockMs;
            m_dbg.coyoteT = 0.f;
            m_dbg.bufferT = 0.f;

            // 전이는 즉시 적용하지 않고 '요청'만 한다
            RequestChange ( std::make_unique<Jump> ( ) , PState::Jump );
        }

        // 충돌
        float nx = 0.f , ny = 0.f;
        c.aabb = c.body->ProposeAABB ( fixedDt , &c.prevBottom , &nx , &ny );
        c.vel = c.body->Velocity ( );
        c.ignoreOneWay = ( m_dbg.dropT > 0.f ) || ( c.vel.y < 0.f );
        c.col->MoveAndCollide ( c.aabb , c.vel , &c.rep , c.ignoreOneWay , c.prevBottom );
        c.body->ApplyCollisionResult ( c.aabb , c.vel , c.rep , nx , ny );

        // 지면 히스테리시스
        if ( c.rep.grounded ) m_dbg.groundHoldT = m_cfg.groundHoldMs;

        // 저점프: 상승 중 점프키 떼면 감쇠
        if ( !c.jumpHeld && c.body->Velocity ( ).y < 0.f ) {
            auto v = c.body->Velocity ( ); v.y *= m_cfg.shortHopMul; c.body->SetVelocity ( v );
        }
    }

    // ====== 전이 관리 ======
    void PlayerFSM::RequestChange ( std::unique_ptr<State> ns , PState tag )
    {
        // 마지막 요청만 유지 (필요 시 우선순위 큐로 확장 가능)
        m_pending = std::move ( ns );
        m_pendingTag = tag;
    }

    void PlayerFSM::ApplyPending ( Ctx& c )
    {
        if ( !m_pending ) return;
        if ( !CanTransition ( m_state , m_pendingTag , c ) ) {
            // 금지된 전이는 폐기
            m_pending.reset ( );
            return;
        }
        if ( m_cur ) m_cur->OnExit ( );
        m_cur = std::move ( m_pending );
        m_state = m_pendingTag;
        m_needEnter = true;
    }

    bool PlayerFSM::CanTransition ( PState from , PState to , const Ctx& c ) const
    {
        if ( from == to ) return false; // 중복 전이 방지

        // 점프 락: Jump -> (Idle/Walk) 금지
        if ( from == PState::Jump && ( to == PState::Idle || to == PState::Walk ) && m_jumpLockT > 0.f )
            return false;

        // 상승 → 하강 전환 규칙: Jump -> Fall 은 vy >= 0 일 때만
        if ( from == PState::Jump && to == PState::Fall && c.body->Velocity ( ).y < 0.f )
            return false;

        // 필요 시 더 많은 규칙을 여기에 추가 (무적, 원웨이, 대시 등)
        return true;
    }

    // ====== 슈퍼 상태 구현 ======
    void PlayerFSM::Grounded::Update ( Ctx& c , PlayerFSM& fsm )
    {
        c.body->SetDesiredRunAxis ( c.ax );

        // 안정 접지(히스테리시스 포함) 해제 → Fall
        const bool stable = c.body->Grounded ( ) || ( fsm.m_dbg.groundHoldT > 0.f );
        if ( !stable ) fsm.RequestChange ( std::make_unique<Fall> ( ) , PState::Fall );
    }

    void PlayerFSM::Airborne::Update ( Ctx& c , PlayerFSM& fsm )
    {
        c.body->SetDesiredRunAxis ( c.ax );

        // 점프 락 중에는 지상 전이를 막고 싶다면 CanTransition이 알아서 거른다.
        const bool stable = c.body->Grounded ( ) || ( fsm.m_dbg.groundHoldT > 0.f );
        if ( stable ) {
            if ( std::fabs ( c.ax ) > 0.1f ) fsm.RequestChange ( std::make_unique<Walk> ( ) , PState::Walk );
            else                        fsm.RequestChange ( std::make_unique<Idle> ( ) , PState::Idle );
        }
    }

    // ====== 리프 상태 구현 ======
    void PlayerFSM::Idle::Update ( Ctx& c , PlayerFSM& fsm )
    {
        c.body->SetDesiredRunAxis ( 0.f );

        const bool stable = c.body->Grounded ( ) || ( fsm.m_dbg.groundHoldT > 0.f );
        if ( !stable ) {
            fsm.RequestChange ( std::make_unique<Fall> ( ) , PState::Fall );
            return;
        }
        if ( std::fabs ( c.ax ) > 0.1f )
            fsm.RequestChange ( std::make_unique<Walk> ( ) , PState::Walk );
    }

    void PlayerFSM::Walk::Update ( Ctx& c , PlayerFSM& fsm )
    {
        Grounded::Update ( c , fsm ); // 지면 이탈 감시
        if ( std::fabs ( c.ax ) <= 0.1f )
            fsm.RequestChange ( std::make_unique<Idle> ( ) , PState::Idle );
    }

    void PlayerFSM::Jump::Update ( Ctx& c , PlayerFSM& fsm )
    {
        c.body->SetDesiredRunAxis ( c.ax );

        // 점프 락 동안은 Jump 유지
        if ( fsm.m_jumpLockT > 0.f ) return;

        // 상승→하강 전환되면 Fall 시도 (실제 적용은 중앙 CanTransition이 승인 시)
        if ( c.body->Velocity ( ).y >= 0.f ) {
            fsm.RequestChange ( std::make_unique<Fall> ( ) , PState::Fall );
            return;
        }

        // 락이 끝났고, 공중 공통 로직(착지)을 허용
        Airborne::Update ( c , fsm );
    }

    void PlayerFSM::Fall::Update ( Ctx& c , PlayerFSM& fsm )
    {
        // 공통 착지 처리
        Airborne::Update ( c , fsm );
    }

} // namespace game
