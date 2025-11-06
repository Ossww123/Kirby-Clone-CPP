#pragma once
//
// Responsibility: Common monster base — physics/collision step, health/knockback, spawn hooks.
// Non-Goals:      Per-monster AI logic (TickAI is abstract), rendering policy.
// Call-Context:   Main thread; fixed update + optional debug draw.
// Notes:          Headers use IntRect/RGBA8 only; no Windows types.
//

#include <string>
#include <functional>

#include "engine/core/Object.h"             // base (complete type needed)
#include "engine/physics/PhysicsBody.h"     // member by value
#include "engine/util/Anim.h"               // member by value
#include "engine/util/Types.h"              // IntRect

#include "game/combat/Damage.h"             // Health/Damage (member + param)

// fwd decls to reduce header deps
namespace engine {
    struct Vec2;
    class  Input;
    namespace physics { class CollisionSystem; }
    class  D3D11DebugDraw; // debug-only param; no header include here
    struct Tex2D;          // stored as pointer only
}

namespace game {
    // forward-declare enums/structs with fixed underlying types
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

        // config
        struct Cfg {
            engine::PhysicsParams phys{};
            bool  ignoreOneWayUpward = true;  // skip one-way while moving upward
            int   maxHp = 2;
            float iFrameMs = 0.3f;
            float knockbackMul = 0.5f;        // global knockback scale
        };

        // lifecycle
        Monster ( const engine::IntRect& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Cfg& cfg = {} );
        virtual ~Monster ( ) = default;

        // frame update (AI in derived, physics/common here)
        void Update ( double fixedDt , const engine::Input& input ) override;

        // debug
        void RenderDebug ( engine::D3D11DebugDraw* dbg , int ox , int oy ) const;

        // state
        bool  Alive ( ) const { return m_alive; }
        void  Kill ( ) { m_alive = false; }

        // inhale metadata
        virtual bool    Inhalable ( )   const { return true; }
        virtual Ability AbilityGift ( ) const; // default: Ability::None

        // combat
        void  OnHit ( const Damage& d );
        void  SetKnockbackMul ( float k ) { m_cfg.knockbackMul = k; }
        float KnockbackMul ( ) const { return m_cfg.knockbackMul; }

        // common utils
        void         SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
        engine::Vec2 Velocity ( ) const { return m_body.Velocity ( ); }
        bool         Grounded ( ) const { return m_body.Grounded ( ); }
        void         GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }

        // callbacks setters
        void SetProjectileSpawnerId ( SpawnProjectileIdFn fn ) { m_spawnProjId = std::move ( fn ); }
        void SetTargetQuery ( QueryTargetPosFn fn ) { m_queryTarget = std::move ( fn ); }
        void SetHitVolumeSpawner ( SpawnHitVolumeFn fn ) { m_spawnHV = std::move ( fn ); }
        void SetMonsterSpawner ( SpawnMonsterFn fn ) { m_spawnMonster = std::move ( fn ); }

        // animation
        engine::Animator* Animator ( ) { return &m_anim; }
        const engine::Animator* Animator ( ) const { return &m_anim; }

        // ===== Sprite (temporary single frame) =====
        void                SetSpriteSheet ( const engine::Tex2D* tex ) { m_tex = tex; }
        void                SetSpriteSrc ( const engine::IntRect& r ) { m_src = r; }
        void                SetVisualSize ( float w , float h ) { m_visW = w; m_visH = h; }
        void                GetVisualSize ( float& w , float& h ) const { w = m_visW; h = m_visH; }
        void                SetSize ( float w , float h ) { m_body.SetSize ( w , h ); }
        const engine::Tex2D* TexturePtr ( ) const { return m_tex; }
        engine::IntRect     SpriteSrc ( )  const { return m_src; }

        engine::Vec2 Center ( ) const;
        int          Facing ( ) const; // +1/-1 (idle→+1)

    protected:
        // derived AI hook — set desired motion (e.g., m_body.SetDesiredRunAxis(..))
        virtual void TickAI ( double fixedDt , const engine::Input& input ) = 0;

        // common physics/collision step (same flow as player)
        void StepPhysics ( double fixedDt );

        // ground edge probe: 2px forward, 2px down
        bool HasGroundAhead ( int dir ) const;

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
        const engine::Tex2D* m_tex{ nullptr }; // owned elsewhere (e.g., enemies.png)
        engine::IntRect      m_src{ 0,0,0,0 }; // sheet rect
        float                m_visW{ 16.f } , m_visH{ 16.f };
    };

} // namespace game
