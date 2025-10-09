#pragma once
#include <string>
#include <cmath>
#include "engine/Input.h"
#include "engine/Time.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Anim.h"

namespace game {

    enum class PState { Idle , Walk , Jump , Fall };

    // 헬퍼: 상태명 문자열
    inline const char* ToString ( PState s ) {
        switch ( s ) {
        case PState::Idle: return "Idle";
        case PState::Walk: return "Walk";
        case PState::Jump: return "Jump";
        case PState::Fall: return "Fall";
        default: return "?";
        }
    }

    // Kirby 미니 FSM: GameApp에서 player Body/Collision을 넘겨 받아 Step만 호출
    class PlayerFSM {
    public:
        struct Cfg {
            float jumpSpeed = 700.f;
            float coyoteMs = 0.08f;
            float bufferMs = 0.10f;
            float dropMs = 0.20f;
            float shortHopMul = 0.45f;   // 저점프 감쇠(상승중 키 뗄 때)
        };

        struct DebugInfo {
            PState state{ PState::Idle };
            bool groundedRaw{ false };
            bool groundedStable{ false };
            bool ignoreOneWay{ false };
            float coyoteT{ 0.f } , bufferT{ 0.f } , dropT{ 0.f } , groundHoldT{ 0.f };
            float vx{ 0.f } , vy{ 0.f };
            RECT  lastAABB{ 0,0,0,0 };
            int   prevBottom{ 0 };
        };

        void Init ( engine::PhysicsBody* body ,
                  const engine::physics::CollisionSystem* worldCol ,
                  engine::Animator* anim = nullptr ,
                  const Cfg& cfg = {} )
        {
            m_body = body;
            m_col = worldCol;
            m_anim = anim;
            m_cfg = cfg;
            m_state = PState::Idle;
            if ( m_anim ) m_anim->Play ( "Idle" , true );
        }

        // 고정 틱에서 호출
        void Step ( double fixedDt , const engine::Input& input )
        {
            if ( !m_body || !m_col ) return;
            const float dt = static_cast< float >( fixedDt );

            // --- 타이머 감소 ---
            m_coyoteT = std::max ( 0.f , m_coyoteT - dt );
            m_bufferT = std::max ( 0.f , m_bufferT - dt );
            m_dropT = std::max ( 0.f , m_dropT - dt );

            // --- 입력 축/액션 수집 ---
            const float ax = input.GetAxis ( "MoveX" );
            const float ay = input.GetAxis ( "MoveY" );
            const bool  jumpPressed = input.ActionPressed ( "Jump" ) || input.Pressed ( VK_SPACE );
            const bool  jumpHeld = input.ActionDown ( "Jump" ) || input.Down ( VK_SPACE );

            // 버퍼: 점프 눌린 순간 저장
            if ( jumpPressed ) m_bufferT = m_cfg.bufferMs;
            // 드롭스루: 지상 + ↓ + 점프
            if ( m_body->Grounded ( ) && ay < -0.5f && jumpPressed ) m_dropT = m_cfg.dropMs;

            // --- 가속/중력(입력 반영은 상태별로) ---
            m_body->AdvanceKinematics ( fixedDt );

            // 코요테 리필
            if ( m_body->Grounded ( ) ) m_coyoteT = m_cfg.coyoteMs;

            // 버퍼된 점프 처리(지상 또는 코요테 중)
            if ( ( m_body->Grounded ( ) || m_coyoteT > 0.f ) && m_bufferT > 0.f ) {
                m_body->Jump ( m_cfg.jumpSpeed );
                m_coyoteT = 0.f;
                m_bufferT = 0.f;
                changeState ( PState::Jump );
            }

            // --- 충돌 처리(원웨이 무시 여부 포함) ---
            int prevBottom = 0;
            RECT aabb = m_body->ProposeAABB ( fixedDt , &prevBottom );
            engine::Vec2 v = m_body->Velocity ( );

            engine::physics::CollisionReport rep{};
            const bool ignoreOneWay = ( m_dropT > 0.f ) || ( v.y < 0.f );
            m_col->MoveAndCollide ( aabb , v , &rep , ignoreOneWay , prevBottom );
            m_body->ApplyCollisionResult ( aabb , v , rep.grounded );

            // 상승 중 점프 키 떼면 저점프 감쇠
            if ( !jumpHeld && m_body->Velocity ( ).y < 0.f ) {
                auto vv = m_body->Velocity ( );
                vv.y *= m_cfg.shortHopMul;
                m_body->SetVelocity ( vv );
            }

            // --- 상태별 입력/전이 ---
            switch ( m_state ) {
            case PState::Idle:
                m_body->SetDesiredRunAxis ( 0.f );          // 기본은 멈춤
                if ( !m_body->Grounded ( ) ) changeState ( PState::Fall );
                else if ( std::fabs ( ax ) > 0.1f ) changeState ( PState::Walk );
                break;

            case PState::Walk:
                m_body->SetDesiredRunAxis ( ax );           // 걷기 중에만 축 적용
                if ( !m_body->Grounded ( ) )      changeState ( PState::Fall );
                else if ( std::fabs ( ax ) <= 0.1f ) changeState ( PState::Idle );
                break;

            case PState::Jump:
                m_body->SetDesiredRunAxis ( ax );
                if ( m_body->Velocity ( ).y >= 0.f ) changeState ( PState::Fall );
                break;

            case PState::Fall:
                m_body->SetDesiredRunAxis ( ax );
                if ( m_body->Grounded ( ) ) changeState ( std::fabs ( ax ) > 0.1f ? PState::Walk : PState::Idle );
                break;
            }

            // ---- 디버그 스냅샷 ----
            m_dbg.state = m_state;
            m_dbg.groundedRaw = rep.grounded;
            // 히스테리시스(30ms): 순간 끊김 완화
            if ( rep.grounded ) m_groundHoldT = 0.03f;
            m_dbg.groundedStable = rep.grounded || ( m_groundHoldT > 0.f );
            m_dbg.ignoreOneWay = ignoreOneWay;
            m_dbg.coyoteT = m_coyoteT;
            m_dbg.bufferT = m_bufferT;
            m_dbg.dropT = m_dropT;
            m_dbg.groundHoldT = m_groundHoldT;
            const auto vel = m_body->Velocity ( );
            m_dbg.vx = vel.x; m_dbg.vy = vel.y;
            m_dbg.lastAABB = aabb;
            m_dbg.prevBottom = prevBottom;
        }

        PState State ( ) const { return m_state; }
        const char* StateName ( ) const { return ToString ( m_state ); }
        DebugInfo GetDebug ( ) const { return m_dbg; }

    private:
        void changeState ( PState s ) {
            if ( m_state == s ) return;
            m_state = s;
            if ( !m_anim ) return;
            switch ( s ) {
            case PState::Idle: m_anim->Play ( "Idle" , false ); break;
            case PState::Walk: m_anim->Play ( "Walk" , false ); break;
            case PState::Jump: m_anim->Play ( "Jump" , true ); break;
            case PState::Fall: m_anim->Play ( "Fall" , false ); break;
            }
        }

        // refs
        engine::PhysicsBody* m_body = nullptr;
        const engine::physics::CollisionSystem* m_col = nullptr;
        engine::Animator* m_anim = nullptr;

        // cfg/state
        Cfg   m_cfg{};
        PState m_state{ PState::Idle };
        float m_coyoteT{ 0.f };
        float m_bufferT{ 0.f };
        float m_dropT{ 0.f };
        float m_groundHoldT{ 0.f };   // grounded 히스테리시스
        DebugInfo m_dbg{};
    };

} // namespace game
