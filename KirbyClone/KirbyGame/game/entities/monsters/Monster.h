#pragma once
//
// Responsibility: Common monster base — physics/collision, health/knockback, spawn hooks.
// Non-Goals:      Rendering and debug drawing; per-monster AI visuals.
// Call-Context:   Main thread; fixed update.
// Notes:          Headers use IntRect/RGBA8 only; no Windows or renderer types.
//

#include <string>
#include <functional>

#include "engine/core/Object.h"            // base (complete)
#include "engine/physics/PhysicsBody.h"    // member by value
#include "engine/util/Anim.h"              // member by value
#include "engine/util/Types.h"             // IntRect
#include "game/combat/Damage.h"            // Health/Damage

// fwd decls to reduce deps
namespace engine {
    struct Vec2;
    class  Input;
    namespace physics { class CollisionSystem; }
}

namespace game {
    enum class Ability : int;
    enum class ProjOwner : int;
    enum class MonsterType : int;
    struct SpawnSpec;

    class Monster : public engine::Object {
    public:
        // callbacks
        using QueryTargetPosFn = std::function<engine::Vec2 ( )>;
        using SpawnProjectileIdFn = std::function<void ( const std::string& archetype ,
                                                       const engine::Vec2& pos ,
                                                       const engine::Vec2& vel ,
                                                       ProjOwner owner )>;
        using SpawnHitVolumeFn = std::function<void ( const std::string& archetype ,
                                                       int ownerId , int facing ,
                                                       const engine::Vec2& anchor )>;
        using SpawnMonsterFn = std::function<void ( MonsterType type ,
                                                       const engine::Vec2& pos ,
                                                       const SpawnSpec& spec )>;

        struct Cfg {
            engine::PhysicsParams phys{};
            bool  ignoreOneWayUpward = true;
            int   maxHp = 2;
            float iFrameMs = 0.3f;
            float knockbackMul = 0.5f;
        };

        Monster ( const engine::IntRect& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Cfg& cfg = {} );
        virtual ~Monster ( ) = default;

        void Update ( double fixedDt , const engine::Input& input ) override;

        // state
        bool  Alive ( ) const { return m_alive; }
        void  Kill ( ) { m_alive = false; }

        // last frame collision flags
        bool HitX ( ) const { return m_lastHitX; }
        bool HitY ( ) const { return m_lastHitY; }

        // inhale metadata
        virtual bool    Inhalable ( )   const { return true; }
        virtual Ability AbilityGift ( ) const; // default: Ability::None

        // combat
        void  OnHit ( const Damage& d );
        void  SetKnockbackMul ( float k ) { m_cfg.knockbackMul = k; }
        float KnockbackMul ( ) const { return m_cfg.knockbackMul; }

        // queries & utils
        void         SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
        engine::Vec2 Velocity ( ) const { return m_body.Velocity ( ); }
        bool         Grounded ( ) const { return m_body.Grounded ( ); }
        void         GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }
        engine::Vec2 Center ( ) const;
        int          Facing ( ) const;  // +1/-1 (idle→+1)
        int          HP ( ) const { return m_health.hp; }
        int          MaxHP ( ) const { return m_health.maxHp; }

        // callbacks setters
        void SetProjectileSpawnerId ( SpawnProjectileIdFn fn ) { m_spawnProjId = std::move ( fn ); }
        void SetTargetQuery ( QueryTargetPosFn fn ) { m_queryTarget = std::move ( fn ); }
        void SetHitVolumeSpawner ( SpawnHitVolumeFn fn ) { m_spawnHV = std::move ( fn ); }
        void SetMonsterSpawner ( SpawnMonsterFn fn ) { m_spawnMonster = std::move ( fn ); }

        // animation handle (no rendering policy here)
        engine::Animator* Animator ( ) { return &m_anim; }
        const engine::Animator* Animator ( ) const { return &m_anim; }

    protected:
        virtual void TickAI ( double fixedDt , const engine::Input& input ) = 0; // derived AI hook
        void        StepPhysics ( double fixedDt );                              // shared physics step
        bool        HasGroundAhead ( int dir ) const;                            // 2px forward probe

    protected:
        engine::PhysicsBody m_body;
        const engine::physics::CollisionSystem* m_col{};
        engine::Animator    m_anim{};

        bool   m_ignoreOneWay = false;
        Health m_health{};
        bool   m_alive = true;
        Cfg    m_cfg{};

        // spawners
        SpawnProjectileIdFn m_spawnProjId;
        QueryTargetPosFn    m_queryTarget;
        SpawnHitVolumeFn    m_spawnHV;
        SpawnMonsterFn      m_spawnMonster;

    private:
        bool  m_lastHitX{ false };
        bool  m_lastHitY{ false };

    };

} // namespace game
