#pragma once
#include <memory>
#include <string>
#include "engine/Object.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Anim.h"
#include "engine/Math.h"

namespace game {

    class Monster : public engine::Object {
    public:
        struct Cfg {
            engine::PhysicsParams phys;
            bool ignoreOneWayUpward = true; // 공중 상승 중 원웨이 무시(보통 몬스터는 무시 안 함)
        };

        Monster ( const RECT& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Cfg& cfg = {} )
            : m_body ( worldBounds , cfg.phys ) , m_col ( col ) , m_cfg ( cfg ) {}

        virtual ~Monster ( ) = default;

        // 프레임 갱신 (물리/충돌은 공통, AI는 파생에서 결정)
        void Update ( double fixedDt , const engine::Input& input ) override {
            // 1) 파생 AI로 이동 의도 계산
            TickAI ( fixedDt , input );

            // 2) 물리/충돌 처리 (공통 루틴)
            StepPhysics ( fixedDt );

            // 3) 애니메이션
            m_anim.Update ( fixedDt );
        }

        // 추후 스프라이트로 교체
        void Render ( HDC , int , int ) override {}

        // --- 공용 유틸 ---
        void SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
        engine::Vec2 Velocity ( ) const { return m_body.Velocity ( ); }
        bool Grounded ( ) const { return m_body.Grounded ( ); }
        void GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }

    protected:
        // 파생이 오버라이드: 이 프레임의 이동 의도/상태 결정(예: m_body.SetDesiredRunAxis(..))
        virtual void TickAI ( double fixedDt , const engine::Input& input ) = 0;

        // 공통 물리/충돌(플레이어와 동일한 흐름)
        void StepPhysics ( double fixedDt ) {
            // 1) 가속/중력
            m_body.AdvanceKinematics ( fixedDt );

            // 2) 충돌 예측/적용
            int prevBottom = 0; float nx = 0.f , ny = 0.f;
            RECT aabb = m_body.ProposeAABB ( fixedDt , &prevBottom , &nx , &ny );

            engine::Vec2 vel = m_body.Velocity ( );
            m_ignoreOneWay = ( m_cfg.ignoreOneWayUpward && vel.y < 0.f );

            m_col->MoveAndCollide ( aabb , vel , &m_rep , m_ignoreOneWay , prevBottom );
            m_body.ApplyCollisionResult ( aabb , vel , m_rep , nx , ny );
        }

        // 바닥 가장자리 감지(앞쪽 2px, 아래 2px 프로브)
        bool HasGroundAhead ( int dir ) const {
            int x , y , w , h; m_body.GetBounds ( x , y , w , h );
            const int probeW = 2;
            RECT probe{
                dir > 0 ? ( x + w + 1 ) : ( x - probeW - 1 ),
                y + h,
                dir > 0 ? ( x + w + 1 + probeW ) : ( x - 1 ),
                y + h + 3
            };
            // 정지/원웨이 모두 검사
            for ( const RECT& s : m_col->Statics ( ) ) if ( engine::physics::Overlap ( probe , s ) ) return true;
            for ( const RECT& o : m_col->OneWays ( ) ) if ( engine::physics::Overlap ( probe , o ) ) return true;
            return false;
        }

    protected:
        engine::PhysicsBody m_body;
        const engine::physics::CollisionSystem* m_col{};
        engine::Animator m_anim;

        // 마지막 충돌 결과(벽 충돌/접지 등)
        engine::physics::CollisionReport m_rep{}; // Collision.h에 정의되어 있음
        bool m_ignoreOneWay = false;

        Cfg m_cfg{};
    };

} // namespace game
