#pragma once
#include <memory>
#include <string>
#include <cmath>
#include "engine/Object.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Math.h"

namespace game {

    enum class ProjOwner { Player , Enemy };

    class Projectile : public engine::Object {
    public:
        struct Cfg {
            float width = 8.f;
            float height = 8.f;
            float speed = 480.f;
            float ttl = 1.5f;   // 초: 수명
            bool  dieOnAnyWorldHit = true; // 벽/바닥/천장 접촉 시 소멸
        };

        Projectile ( const RECT& worldBounds ,
                   const engine::physics::CollisionSystem* col ,
                   ProjOwner owner ,
                   const Cfg& cfg = {} )
            : m_body ( worldBounds , engine::PhysicsParams{} ) ,
            m_col ( col ) , m_cfg ( cfg ) , m_owner ( owner )
        {
            m_body.SetSize ( m_cfg.width , m_cfg.height );
        }

        void Fire ( const engine::Vec2& pos , const engine::Vec2& vel ) {
            m_body.SetPosition ( pos.x , pos.y );
            m_body.SetVelocity ( vel );
            m_alive = true;
            m_ttl = m_cfg.ttl;
        }

        // ---- Object ----
        void Update ( double fixedDt , const engine::Input& ) override {
            if ( !m_alive ) return;

            m_ttl -= static_cast< float >( fixedDt );
            if ( m_ttl <= 0.f ) { m_alive = false; return; }

            // 중력 없이 직진: 가끔 약간 떨어뜨리고 싶으면 PhysicsParams.gravity 쓰면 됨
            m_body.AdvanceKinematics ( fixedDt );

            int prevBottom = 0; float nx = 0.f , ny = 0.f;
            RECT aabb = m_body.ProposeAABB ( fixedDt , &prevBottom , &nx , &ny );
            auto vel = m_body.Velocity ( );
            engine::physics::CollisionReport rep{};
            // 상승 중에도 원웨이는 무시: 관통 원하면 false로
            const bool ignoreOneWay = ( vel.y < 0.f );

            m_col->MoveAndCollide ( aabb , vel , &rep , ignoreOneWay , prevBottom );
            m_body.ApplyCollisionResult ( aabb , vel , rep , nx , ny );

            if ( m_cfg.dieOnAnyWorldHit && ( rep.hitX || rep.hitY || rep.grounded ) ) {
                m_alive = false;
            }
        }

        void Render ( HDC , int , int ) override {}

        // ---- Query ----
        bool Alive ( ) const { return m_alive; }
        ProjOwner Owner ( ) const { return m_owner; }
        void Kill ( ) { m_alive = false; }
        void GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }

    private:
        engine::PhysicsBody m_body;
        const engine::physics::CollisionSystem* m_col{};
        Cfg m_cfg{};
        ProjOwner m_owner{ ProjOwner::Player };
        bool  m_alive{ false };
        float m_ttl{ 0.f };
    };

} // namespace game
