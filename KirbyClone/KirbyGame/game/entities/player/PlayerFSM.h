#pragma once
//
// Responsibility: Player finite state machine (movement/action/overlay tracks),
//                 events out, damage and persistence snapshot.
// Non-Goals:      Rendering/audio; platform I/O plumbing.
// Call-Context:   Main thread; driven by fixed-step loop.
//

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#include "engine/core/Input.h"
#include "engine/physics/PhysicsBody.h"
#include "engine/physics/Collision.h"   // CollisionReport
#include "engine/util/Math.h"           // Vec2
#include "engine/util/Types.h"          // IntRect
#include "game/combat/Damage.h"         // Team / HitKind / Damage / Health
#include "game/combat/Ability.h"

namespace engine { class Animator; }

namespace game {

    // ---- Parallel tracks ----
    enum class MState { Idle , Walk , Run , Crouch , Slide , Jump , Fall , Inflated , Ladder };
    enum class AState { Neutral , Inhale , MouthFull , SpitObject , AirPuff , AbilityAtk, WaterShot };
    enum class ZState { None , Damaged , Dead , DoorEnter , Dance , GameOver };

    // ---- Events from FSM to Game/World ----
    struct PlayerEvent {
        enum Type {
            InhaleVolume , SpitStar , AirPuffShot , WaterShot , SwallowAbility , AbilityGained ,
            AbilityFire , AbilitySpark , AbilityBeam ,
            DoorInteract,
            Died
        } type;
        engine::IntRect rect{};         // world-space AABB (for InhaleVolume)
        int             facing{ +1 };   // +1 right, -1 left
        int             dx{ 0 };        // shot x (-1,0,+1)
        int             dy{ 0 };        // shot y (-1,0,+1)
        Ability         ability{ Ability::None };
    };

    // ---- ToString helpers (Movement/Action/Overlay) ----
    inline const char* ToString ( MState s ) {
        switch ( s ) {
        case MState::Idle: return "Idle"; case MState::Walk: return "Walk";
        case MState::Run: return "Run";   case MState::Crouch: return "Crouch";
        case MState::Slide: return "Slide"; case MState::Jump: return "Jump";
        case MState::Fall: return "Fall"; case MState::Inflated: return "Inflated";
        case MState::Ladder: return "Ladder"; default: return "?";
        }
    }
    inline const char* ToString ( AState s ) {
        switch ( s ) {
        case AState::Neutral: return "Neutral"; case AState::Inhale: return "Inhale";
        case AState::MouthFull: return "MouthFull"; case AState::SpitObject: return "SpitObject";
        case AState::WaterShot:  return "WaterShot";
        case AState::AirPuff: return "AirPuff"; case AState::AbilityAtk: return "AbilityAtk";
        default: return "?";
        }
    }
    inline const char* ToString ( ZState s ) {
        switch ( s ) {
        case ZState::None: return "None"; case ZState::Damaged: return "Damaged";
        case ZState::Dead: return "Dead"; case ZState::DoorEnter: return "DoorEnter";
        case ZState::Dance: return "Dance"; case ZState::GameOver: return "GameOver";
        default: return "?";
        }
    }

    class PlayerFSM {
    public:
        struct Cfg {
            // movement/physics tuning
            float jumpSpeed = 700.f;
            float coyoteMs = 0.08f;
            float bufferMs = 0.10f;
            float dropMs = 0.20f;
            float shortHopMul = 0.45f;
            float groundHoldMs = 0.033f;
            float jumpLockMs = 0.03f;
            int   maxTransitionsPerStep = 3;
            // combat/health
            int   maxHp = 6;
            float iFrameMs = 0.8f;
            float damagedStun = 0.25f;
            float hurtKnockbackClamp = 520.f;
            // fall phases / long-fall & bounce
            float fallTumbleMs = 0.30f;      // FALL0 duration after jump→fall
            float fallLongMs = 0.70f;        // time threshold for FALL2
            float fallLongHeightPx = 400.f;  // height threshold for FALL2
            float bounceSpeedUp = 400.f;     // initial up-speed on long-fall bounce
            // water movement
            float waterWalkSpeed = 80.f;
            float waterRunSpeed = 0.f;  // not used
            float swimSpeed = 110.f;    
            float waterGravity = 500.f; 
            float waterDrag = 6.f;      
        };

        // shared tuning constants (available across .cpp)
        static constexpr float TAP_WINDOW = 0.28f;  // run double-tap window
        static constexpr float RUN_TOGGLE_AX = 0.10f;  // axis deadzone
        static constexpr float SLIDE_TIME = 0.22f;
        static constexpr float SLIDE_VX = 280.f;
        static constexpr float FLAP_VY = -240.f;
        static constexpr float FLOAT_EXIT_VY = 60.f;

        struct DebugInfo {
            // physics snapshot
            bool groundedRaw{ false };
            bool groundedStable{ false };
            bool ignoreOneWay{ false };
            float coyoteT{ 0.f } , bufferT{ 0.f } , dropT{ 0.f } , groundHoldT{ 0.f };
            float vx{ 0.f } , vy{ 0.f };
            engine::IntRect lastAABB{};
            int prevBottom{ 0 };
            // --- environment ---
            bool inWater{ false };
            bool onLadder{ false };
            // health
            int   hp{ 0 };
            float iFrameT{ 0.f };
            // tracks
            MState mState{ MState::Idle };
            AState aState{ AState::Neutral };
            ZState zState{ ZState::None };
            // player flags
            int     facing{ +1 };
            bool    mouthFull{ false };
            Ability ability{ Ability::None };
            // action timers
            float inhaleT{ 0.f };
            float spitLockT{ 0.f };
            // fall debug
            float fallT{ 0.f };
            bool  longFall{ false };
        };

        // persistent snapshot (for stage transition)
        struct Persistent {
            int     hp = 0;
            Ability ability = Ability::None;
            int     facing = +1;
            bool    mouthFull = false;
        };

        // ===== Public API =====
        void Init ( engine::PhysicsBody* body ,
                   const engine::physics::CollisionSystem* worldCol ,
                   engine::Animator* anim = nullptr ,
                   const Cfg& cfg = {} );

        void Step ( double fixedDt , const engine::Input& input );

        // respawn
        void ResetForRespawn ( );

        // accessors
        MState MoveState ( ) const { return m_mState; }
        AState ActState ( )  const { return m_aState; }
        ZState OverlayState ( ) const { return m_zState; }
        int    Facing ( ) const { return m_facing; }
        DebugInfo GetDebug ( ) const { return m_dbg; }

        // setting
        void SetFacing ( int dir ) { m_facing = ( dir < 0 ? -1 : +1 ); m_dbg.facing = m_facing; };

        // damage in
        bool ApplyDamage ( const Damage& d );

        // world -> FSM
        void OnMouthCatch ( Ability gift );

        // events out (one-frame queue)
        void DrainEvents ( std::vector<PlayerEvent>& out ) { out = std::move ( m_events ); m_events.clear ( ); }

        // state names (for HUD/debug)
        const char* MoveStateName ( )    const { return ToString ( m_mState ); }
        const char* ActionStateName ( )  const { return ToString ( m_aState ); }
        const char* OverlayStateName ( ) const { return ToString ( m_zState ); }

        std::string StateNameCombined ( ) const {
            return std::string ( "M:" ) + ToString ( m_mState )
                + " | A:" + ToString ( m_aState )
                + " | Z:" + ToString ( m_zState );
        }

        // overlay control
        void BeginDoorEnter ( );
        void EndDoorEnter ( );
        void BeginDance ( );   // enter Z_Dance
        void EndDance ( );     // back to Z_None
        void BeginGameOver ( );

        // persistence I/O
        Persistent SnapshotPersistent ( ) const;
        void       RestorePersistent ( const Persistent& s );

        int  Hp ( ) const noexcept { return m_health.hp; }
        int  MaxHp ( ) const noexcept { return m_cfg.maxHp; }

    private:
        // per-frame context
        struct Ctx {
            // refs
            engine::PhysicsBody* body{};
            const engine::physics::CollisionSystem* col{};
            engine::Animator* anim{};
            Cfg                                        cfg{};
            // time & input
            float dt{ 0.f };
            float ax{ 0.f } , ay{ 0.f };
            bool  jumpPressed{ false } , jumpHeld{ false };
            bool  attackPressed{ false } , attackHeld{ false };
            bool  abilityPressed{ false } , interactPressed{ false };
            // physics snapshot
            engine::IntRect                 aabb{};
            int                             prevBottom{ 0 };
            engine::Vec2                    vel{ 0.f, 0.f };
            engine::physics::CollisionReport rep{};
            bool                            ignoreOneWay{ false };
            // environment
            bool inWater{ false };
            bool waterGround{ false };
            bool underwater{ false };
            bool onLadder{ false };
            // modifiers from Action -> Movement
            struct Mod { float runAxisMul = 1.f; bool lockRunAxis = false; } mod;
        };

        // base interfaces per track
        struct MBase { virtual ~MBase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };
        struct ABase { virtual ~ABase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };
        struct ZBase { virtual ~ZBase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };

        // movement states (declare; define in .cpps)
        struct M_Grounded   : MBase         { void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Airborne   : MBase         { void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Idle       : M_Grounded    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Walk       : M_Grounded    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Run        : M_Grounded    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Crouch     : M_Grounded    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Slide      : M_Grounded    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Jump       : M_Airborne    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Fall       : M_Airborne    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Inflated   : M_Airborne    { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct M_Ladder     : MBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

        // action states
        struct A_Neutral    : ABase         { void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_Inhale     : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_MouthFull  : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_SpitObject : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_AirPuff    : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_AbilityAtk : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct A_WaterShot  : ABase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

        // overlay states
        struct Z_None       : ZBase         { void Update ( Ctx& , PlayerFSM& ) override; };
        struct Z_Damaged    : ZBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct Z_Dead       : ZBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct Z_DoorEnter  : ZBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct Z_Dance      : ZBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
        struct Z_GameOver   : ZBase         { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

        // utilities
        void UpdateFacing ( const Ctx& c );
        engine::IntRect MakeInhaleBox ( const Ctx& c ) const;
        void ResetFallAccumulators ( );
        void HardResetRuntime ( );

        // transitions per track
        void RequestMove ( std::unique_ptr<MBase> ns , MState tag );
        void RequestAct ( std::unique_ptr<ABase> ns , AState tag );
        void RequestOver ( std::unique_ptr<ZBase> ns , ZState tag );
        void ApplyPendingMove ( Ctx& c );
        void ApplyPendingAct ( Ctx& c );
        void ApplyPendingOver ( Ctx& c );
        bool CanMove ( MState from , MState to , const Ctx& c ) const;
        bool CanAct ( AState from , AState to , const Ctx& c ) const;
        bool CanOver ( ZState from , ZState to , const Ctx& c ) const;

        // physics/collision
        void IntegrateAndCollide ( double fixedDt , const engine::Input& input , Ctx& c );

    private:
        // refs
        engine::PhysicsBody* m_body{};
        const engine::physics::CollisionSystem* m_col{};
        engine::Animator* m_anim{};

        // config
        Cfg m_cfg{};

        // track instances
        std::unique_ptr<MBase> m_move;    MState m_mState{ MState::Idle };
        std::unique_ptr<ABase> m_action;  AState m_aState{ AState::Neutral };
        std::unique_ptr<ZBase> m_overlay; ZState m_zState{ ZState::None };

        // pending
        std::unique_ptr<MBase> m_mPending; MState m_mPendingTag{ MState::Idle };
        std::unique_ptr<ABase> m_aPending; AState m_aPendingTag{ AState::Neutral };
        std::unique_ptr<ZBase> m_zPending; ZState m_zPendingTag{ ZState::None };

        // flow/timers
        bool  m_mNeedEnter{ false } , m_aNeedEnter{ false } , m_zNeedEnter{ false };
        int   m_transitionBudget{ 0 };
        float m_jumpLockT{ 0.f };
        float m_damagedT{ 0.f };
        float m_inhaleT{ 0.f };
        float m_spitLockT{ 0.f };
        float m_tapT{ 0.f };
        float m_slideT{ 0.f };

        // action data
        bool    m_mouthFull{ false };
        int     m_facing{ +1 };
        Ability m_ability{ Ability::None };
        Ability m_caughtGift{ Ability::None };
        int     m_lastTapDir{ 0 };     // -1/0/+1
        bool    m_runQueued{ false };
        float   m_prevAx{ 0.f }; // axis edge detection for double-tap

        // one-shot flags
        bool  m_spitEmitted{ false };

        // fall phases / bounce
        float m_fallT{ 0.f };
        float m_fallY0{ 0.f };
        float m_tumbleT{ 0.f };
        bool  m_fellFromJump{ false };
        bool  m_inLongFall{ false };
        bool  m_bounceQueued{ false };

        // health
        Health     m_health{};
        engine::Vec2 m_pendingKB{ 0.f, 0.f };

        // out events
        std::vector<PlayerEvent> m_events;

        // debug snapshot
        DebugInfo m_dbg{};
    };

} // namespace game
