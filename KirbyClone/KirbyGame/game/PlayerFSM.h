#pragma once
#include <string>
#include <memory>
#include "engine/Input.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Anim.h"

namespace game {

    enum class PState { Idle , Walk , Jump , Fall };

    inline const char* ToString ( PState s ) {
        switch ( s ) {
        case PState::Idle: return "Idle";
        case PState::Walk: return "Walk";
        case PState::Jump: return "Jump";
        case PState::Fall: return "Fall";
        default: return "?";
        }
    }

    class PlayerFSM {
    public:
        struct Cfg {
            float jumpSpeed = 700.f;
            float coyoteMs = 0.08f;
            float bufferMs = 0.10f;
            float dropMs = 0.20f;
            float shortHopMul = 0.45f;
            float groundHoldMs = 0.033f; // 접지 히스테리시스
            float jumpLockMs = 0.03f;  // 점프 직후 전이 잠금
            int   maxTransitionsPerStep = 3; // 한 스텝에서 허용할 최대 전이 수(무한루프 방지)
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

        // --- 외부 인터페이스 ---
        void Init ( engine::PhysicsBody* body ,
                  const engine::physics::CollisionSystem* worldCol ,
                  engine::Animator* anim = nullptr ,
                  const Cfg& cfg = {} );

        void Step ( double fixedDt , const engine::Input& input );

        PState GetState ( ) const { return m_state; }
        const char* StateName ( ) const { return ToString ( m_state ); }
        DebugInfo GetDebug ( ) const { return m_dbg; }

    private:
        // ===== HFSM 내부 지원 구조 =====
        struct Ctx {
            // refs
            engine::PhysicsBody* body{};
            const engine::physics::CollisionSystem* col{};
            engine::Animator* anim{};
            Cfg cfg{};

            // 입력/시간
            float dt{ 0.f };
            float ax{ 0.f } , ay{ 0.f };
            bool  jumpPressed{ false } , jumpHeld{ false };

            // 물리/충돌 스냅샷
            RECT aabb{ 0,0,0,0 };
            int  prevBottom{ 0 };
            engine::Vec2 vel{ 0.f,0.f };
            engine::physics::CollisionReport rep{};

            // 원웨이 무시 여부
            bool ignoreOneWay{ false };
        };

        struct State {
            virtual ~State ( ) = default;
            virtual void OnEnter ( Ctx& ) {}
            virtual void OnExit ( ) {}
            virtual void Update ( Ctx& , PlayerFSM& ) = 0;
            static void Play ( engine::Animator* a , const char* name , bool reset = false ) {
                if ( a ) a->Play ( name , reset );
            }
        };

        // 슈퍼 상태
        struct Grounded : State {
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };
        struct Airborne : State {
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };

        // 리프 상태
        struct Idle : Grounded {
            void OnEnter ( Ctx& c ) override { Play ( c.anim , "Idle" , true ); }
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };
        struct Walk : Grounded {
            void OnEnter ( Ctx& c ) override { Play ( c.anim , "Walk" , true ); }
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };
        struct Jump : Airborne {
            void OnEnter ( Ctx& c ) override { Play ( c.anim , "Jump" , true ); }
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };
        struct Fall : Airborne {
            void OnEnter ( Ctx& c ) override { Play ( c.anim , "Fall" , true ); }
            void Update ( Ctx& c , PlayerFSM& fsm ) override;
        };

        // ===== 전이 관리(중앙집중) =====
        void RequestChange ( std::unique_ptr<State> ns , PState tag ); // 상태 내부/시스템에서 호출
        void ApplyPending ( Ctx& c );                                 // Step 루프에서만 호출
        bool CanTransition ( PState from , PState to , const Ctx& c ) const;

        // ===== 공통 시스템 스텝 =====
        void IntegrateAndCollide ( double fixedDt , const engine::Input& input , Ctx& c );

    private:
        // refs
        engine::PhysicsBody* m_body{};
        const engine::physics::CollisionSystem* m_col{};
        engine::Animator* m_anim{};

        // config/state
        Cfg m_cfg{};
        PState m_state{ PState::Idle };
        std::unique_ptr<State> m_cur;

        // pending transition
        std::unique_ptr<State> m_pending;
        PState m_pendingTag{ PState::Idle };

        // flow control
        bool  m_needEnter{ false };
        int   m_transitionBudget{ 0 };
        float m_jumpLockT{ 0.f };

        // debug
        DebugInfo m_dbg{};
    };

} // namespace game
