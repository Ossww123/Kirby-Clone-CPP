#pragma once
#include <memory>
#include <string>
#include <functional>
#include "engine/Object.h"
#include "engine/PhysicsBody.h"
#include "engine/Collision.h"
#include "engine/Anim.h"
#include "engine/Math.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/Texture.h"
#include "game/Damage.h"

namespace game {
    enum class ProjOwner;

    class Monster : public engine::Object {
    public:
        // 콜백 타입
        using SpawnProjectileFn = std::function<void ( const engine::Vec2& pos ,
                                                     const engine::Vec2& vel ,
                                                     ProjOwner owner )>;
        using QueryTargetPosFn = std::function<engine::Vec2 ( )>; // 예: 플레이어 센터

        struct Cfg {
            engine::PhysicsParams phys;
            bool ignoreOneWayUpward = true; // 공중 상승 중 원웨이 무시(보통 몬스터는 무시 안 함)
            int  maxHp = 2;               // 기본 체력
            float iFrameMs = 0.3f;        // 피격 후 무적 시간
        };

        Monster ( const RECT& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Cfg& cfg = {} )
            : m_body ( worldBounds , cfg.phys ) , m_col ( col ) , m_cfg ( cfg )
        {
            m_health.Reset ( cfg.maxHp , cfg.iFrameMs );
        }

        virtual ~Monster ( ) = default;

        // 프레임 갱신 (물리/충돌은 공통, AI는 파생에서 결정)
        void Update ( double fixedDt , const engine::Input& input ) override {
            if ( !m_alive ) return;

            // 1) 파생 AI로 이동 의도 계산
            TickAI ( fixedDt , input );

            // 2) 물리/충돌 처리 (공통 루틴)
            StepPhysics ( fixedDt );

            // 3) 애니메이션
            m_anim.Update ( fixedDt );


            // 4) 무적 타이머 감소
            m_health.Tick ( static_cast< float >( fixedDt ) );
        }

        void RenderDebug ( engine::D3D11DebugDraw* dbg , int ox , int oy ) const {
            if ( !m_alive ) return;
            int x , y , w , h; m_body.GetBounds ( x , y , w , h );
            dbg->WorldRect ( x , y , w , h , ox , oy , RGB ( 240 , 120 , 60 ) );
            // 체력 표시선(디버그)
            if ( m_health.hp < m_health.maxHp ) {
                const int len = ( int ) ( ( float ) m_health.hp / m_health.maxHp * w );
                dbg->WorldLine ( x , y - 2 , x + len , y - 2 , ox , oy , RGB ( 255 , 60 , 60 ) );
            }
        }

        bool Alive ( ) const { return m_alive; }
        void Kill ( ) { m_alive = false; }

        // 피격 처리 (Projectile 등에서 호출)
        virtual void OnHit ( const Damage& d ) {
            if ( !m_alive ) return;
            if ( !m_health.Apply ( d.amount ) ) return; // 무적 or 사망 시 이미 처리됨
            // 넉백 (기본은 단순 적용)
            engine::Vec2 v = m_body.Velocity ( );
            v.x += d.knockback.x * 0.5f;
            v.y += d.knockback.y * 0.5f;
            m_body.SetVelocity ( v );
            if ( m_health.hp <= 0 ) {
                m_alive = false;
            }
        }

        // 추후 스프라이트로 교체
        void Render ( HDC , int , int ) override {}

        // --- 공용 유틸 ---
        void SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
        engine::Vec2 Velocity ( ) const { return m_body.Velocity ( ); }
        bool Grounded ( ) const { return m_body.Grounded ( ); }
        void GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }

        // 콜백 설정자
        void SetProjectileSpawner ( SpawnProjectileFn fn ) { m_spawnProj = std::move ( fn ); }
        void SetTargetQuery ( QueryTargetPosFn fn ) { m_queryTarget = std::move ( fn ); }

        // ===== Sprite (임시 단일 프레임) =====
    public:
        void SetSpriteSheet ( const engine::Tex2D * tex ) { m_tex = tex; }
        void SetSpriteSrc ( const RECT & r ) { m_src = r; }
        void SetVisualSize ( float w , float h ) { m_visW = w; m_visH = h; }
        void GetVisualSize ( float& w , float& h ) const { w = m_visW; h = m_visH; }
        const engine::Tex2D * TexturePtr ( ) const { return m_tex; }
        RECT SpriteSrc ( ) const { return m_src; }

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

        engine::physics::CollisionReport m_rep{};
        bool m_ignoreOneWay = false;

        Health m_health{};
        bool m_alive = true;
        Cfg m_cfg{};

        SpawnProjectileFn m_spawnProj;  
        QueryTargetPosFn  m_queryTarget;

    private:
        const engine::Tex2D * m_tex{ nullptr }; // enemies.png (GameApp 소유)
        RECT  m_src{ 0,0,0,0 };                // 시트 내 사각형
        float m_visW{ 16.f } , m_visH{ 16.f };  // 화면 표시 크기
    };

} // namespace game
