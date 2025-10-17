#pragma once
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include "engine/Input.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Anim.h"
#include "game/Damage.h" // Team / HitKind / Damage / Health

namespace game {

	// ---- Thin ability enum (expand later) ----
	enum class Ability { None , Fire , Spark , Beam };

	// ---- Parallel tracks ----
	enum class MState { Idle , Walk , Run , Crouch , Slide , Jump , Fall , Inflated , Ladder };
	enum class AState { Neutral , Inhale , MouthFull , SpitObject , AirPuff , AbilityAtk };
	enum class ZState { None , Damaged , Dead , DoorEnter , Dance , GameOver };

	// ---- Events from FSM to Game/World ----
	struct PlayerEvent {
		enum Type { InhaleVolume , SpitStar , AirPuffShot , SwallowAbility , AbilityGained } type;
		RECT rect{}; // world-space AABB (for InhaleVolume)
		int facing{ +1 }; // +1 right, -1 left
		Ability ability{ Ability::None };
	};

	// ---- ToString helpers (Movement/Action/Overlay) ----
	inline const char* ToString ( MState s ) {
		switch ( s ) {
		case MState::Idle: return "Idle";
		case MState::Walk: return "Walk";
		case MState::Run: return "Run";
		case MState::Crouch: return "Crouch";
		case MState::Slide: return "Slide";
		case MState::Jump: return "Jump";
		case MState::Fall: return "Fall";
		case MState::Inflated: return "Inflated";
		case MState::Ladder: return "Ladder";
		default: return "?";
		}
	}
	inline const char* ToString ( AState s ) {
		switch ( s ) {
		case AState::Neutral: return "Neutral";
		case AState::Inhale: return "Inhale";
		case AState::MouthFull: return "MouthFull";
		case AState::SpitObject: return "SpitObject";
		case AState::AirPuff: return "AirPuff";
		case AState::AbilityAtk: return "AbilityAtk";
		default: return "?";
		}
	}
	inline const char* ToString ( ZState s ) {
		switch ( s ) {
		case ZState::None: return "None";
		case ZState::Damaged: return "Damaged";
		case ZState::Dead: return "Dead";
		case ZState::DoorEnter: return "DoorEnter";
		case ZState::Dance: return "Dance";
		case ZState::GameOver: return "GameOver";
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
			int maxTransitionsPerStep = 3;
			// combat/health
			int maxHp = 3;
			float iFrameMs = 0.8f;
			float damagedStun = 0.25f;
			float hurtKnockbackClamp = 520.f;
		};


		struct DebugInfo {
			// physics snapshot
			bool groundedRaw{ false };
			bool groundedStable{ false };
			bool ignoreOneWay{ false };
			float coyoteT{ 0.f } , bufferT{ 0.f } , dropT{ 0.f } , groundHoldT{ 0.f };
			float vx{ 0.f } , vy{ 0.f };
			RECT lastAABB{ 0,0,0,0 };
			int prevBottom{ 0 };
			// health
			int hp{ 0 };
			float iFrameT{ 0.f };
			// track/state debug
			MState mState{ MState::Idle };
			AState aState{ AState::Neutral };
			ZState zState{ ZState::None };
			// player flags
			int facing{ +1 };
			bool mouthFull{ false };
			Ability ability{ Ability::None };
			// action timers
			float inhaleT{ 0.f };
			float spitLockT{ 0.f };
		};

		// ===== Public API =====
		void Init ( engine::PhysicsBody* body ,
		const engine::physics::CollisionSystem* worldCol ,
		engine::Animator* anim = nullptr ,
		const Cfg& cfg = {} );

		void Step ( double fixedDt , const engine::Input& input );

		// accessors
		MState MoveState ( ) const { return m_mState; }
		AState ActState ( ) const { return m_aState; }
		ZState OverlayState ( ) const { return m_zState; }
		int Facing ( ) const { return m_facing; }
		DebugInfo GetDebug ( ) const { return m_dbg; }

		// damage in
		bool ApplyDamage ( const Damage& d );

		// world -> FSM: when an inhaled entity actually reaches Kirby's mouth
		void OnMouthCatch ( Ability gift );

		// events out (one-frame queue)
		void DrainEvents ( std::vector<PlayerEvent>& out ) { out = std::move ( m_events ); m_events.clear ( ); }

		// === explicit names per track ===
		const char* MoveStateName ( ) const { return ToString ( m_mState ); }
		const char* ActionStateName ( ) const { return ToString ( m_aState ); }
		const char* OverlayStateName ( ) const { return ToString ( m_zState ); }

		// === combined string for debugging/HUD ===
		std::string StateNameCombined ( ) const {
			return std::string ( "M:" ) + ToString ( m_mState )
				+ " | A:" + ToString ( m_aState )
				+ " | Z:" + ToString ( m_zState );
		}

	private:
		// ===== Per-frame context =====
		struct Ctx {
			// refs
			engine::PhysicsBody* body{};
			const engine::physics::CollisionSystem* col{};
			engine::Animator* anim{};
			Cfg cfg{};
			// time & input
			float dt{ 0.f };
			float ax{ 0.f } , ay{ 0.f };
			bool jumpPressed{ false } , jumpHeld{ false };
			bool attackPressed{ false } , attackHeld{ false };
			bool abilityPressed{ false } , interactPressed{ false };
			// physics snapshot
			RECT aabb{ 0,0,0,0 };
			int prevBottom{ 0 };
			engine::Vec2 vel{ 0.f,0.f };
			engine::physics::CollisionReport rep{};
			bool ignoreOneWay{ false };
			// modifiers from Action -> Movement
			struct Mod { float runAxisMul = 1.f; bool lockRunAxis = false; } mod;
		};

		// ===== Base interfaces per track =====
		struct MBase { virtual ~MBase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };
		struct ABase { virtual ~ABase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };
		struct ZBase { virtual ~ZBase ( ) = default; virtual void OnEnter ( Ctx& ) {}; virtual void OnExit ( ) {}; virtual void Update ( Ctx& , PlayerFSM& ) = 0; };

		// ===== Movement states (declare; define in .cpp) =====
		struct M_Grounded : MBase { void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Airborne : MBase { void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Idle : M_Grounded { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Walk : M_Grounded { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Run : M_Grounded { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Crouch : M_Grounded { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Slide : M_Grounded { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Jump : M_Airborne { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Fall : M_Airborne { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Inflated : M_Airborne { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct M_Ladder : MBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

		// ===== Action states =====
		struct A_Neutral : ABase { void Update ( Ctx& , PlayerFSM& ) override; };
		struct A_Inhale : ABase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct A_MouthFull : ABase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct A_SpitObject : ABase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct A_AirPuff : ABase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct A_AbilityAtk : ABase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

		// ===== Overlay states =====
		struct Z_None : ZBase { void Update ( Ctx& , PlayerFSM& ) override; };
		struct Z_Damaged : ZBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct Z_Dead : ZBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct Z_DoorEnter : ZBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct Z_Dance : ZBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };
		struct Z_GameOver : ZBase { void OnEnter ( Ctx& ) override; void Update ( Ctx& , PlayerFSM& ) override; };

		// ===== Utilities =====
		static void Play ( engine::Animator* a , const char* name , bool reset = false ) { if ( a ) a->Play ( name , reset ); }
		void UpdateFacing ( const Ctx& c );
		RECT MakeInhaleBox ( const Ctx& c ) const;

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
			std::unique_ptr<MBase> m_move; MState m_mState{ MState::Idle };
			std::unique_ptr<ABase> m_action; AState m_aState{ AState::Neutral };
			std::unique_ptr<ZBase> m_overlay; ZState m_zState{ ZState::None };

			// pending
			std::unique_ptr<MBase> m_mPending; MState m_mPendingTag{ MState::Idle };
			std::unique_ptr<ABase> m_aPending; AState m_aPendingTag{ AState::Neutral };
			std::unique_ptr<ZBase> m_zPending; ZState m_zPendingTag{ ZState::None };

			// flow/timers
			bool m_mNeedEnter{ false } , m_aNeedEnter{ false } , m_zNeedEnter{ false };
			int m_transitionBudget{ 0 };
			float m_jumpLockT{ 0.f };
			float m_damagedT{ 0.f };
			float m_inhaleT{ 0.f };
			float m_spitLockT{ 0.f };
			float m_tapT{ 0.f };
			float m_slideT{ 0.f };

			// action data
			bool m_mouthFull{ false };
			int m_facing{ +1 };
			Ability m_ability{ Ability::None };
			Ability m_caughtGift{ Ability::None };
			int   m_lastTapDir{ 0 };     // -1/0/+1 : 좌/없음/우
			bool  m_runQueued{ false };
			

			// health
			Health m_health{};
			engine::Vec2 m_pendingKB{ 0.f,0.f };

			// out events
			std::vector<PlayerEvent> m_events;

			// debug snapshot
			DebugInfo m_dbg{};
	};

} // namespace game
