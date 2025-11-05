#pragma once
#include "game/Monster.h"

namespace game {

    class WhispyWoods : public Monster {
    public:
        struct Config {
            Monster::Cfg base{ .knockbackMul = 0.f };
            // 공기포(투사체) 패턴
            int   puffVolleyCount = 3;
            float puffIntervalMs = 0.33f;
            float puffSpeed = 220.f;
            float puffRestMs = 1.4f;   // 연사 후 휴식
            // 사과 드랍 패턴
            int   applesPerWave = 3;
            float appleSpanPx = 240.f;  // 보스 중심 기준 좌우로 분산
            float appleTelegraphMs = 0.65f;
            float appleRestMs = 2.8f;
        };

        WhispyWoods ( const RECT& worldBounds ,
                    const engine::physics::CollisionSystem* col ,
                    const Config& cfg ,
                    float x , float y )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
        {
            SetPosition ( x , y );
            m_state = State::Rest;
            m_timer = 0.5f; // 시작 전 짧은 텀
        }

        bool Inhalable ( ) const override { return false; } // 보스는 흡입 불가
        Ability AbilityGift ( ) const override { return Ability::None; }

        void TickAI ( double dt , const engine::Input& ) override {
            // 고정형: 움직임 없음
            m_body.SetDesiredRunAxis ( 0.f );

            m_timer -= static_cast< float >( dt );
            if ( m_timer > 0.f ) return;

            switch ( m_state ) {
            case State::Rest:
                // 간단히 번갈아가며: Puff → Rest → Apple → Rest ...
                if ( m_flip ) { beginPuffVolley ( ); }
                else { beginAppleDrop ( ); }
                m_flip = !m_flip;
                break;

            case State::Puffing:
                if ( m_shotsLeft > 0 ) {
                    if ( m_innerT <= 0.f ) {
                        firePuffOnce ( );
                        m_shotsLeft--;
                        m_innerT = m_cfg.puffIntervalMs;
                    }
                    else {
                        m_innerT -= static_cast< float >( dt );
                    }
                }
                else {
                    m_state = State::Rest; m_timer = m_cfg.puffRestMs;
                }
                break;

            case State::AppleDrop:
                // 한 번에 N개 드랍 후 휴식
                dropApplesOnce ( );
                m_state = State::Rest; m_timer = m_cfg.appleRestMs;
                break;
            }
        }

    private:
        enum class State { Rest , Puffing , AppleDrop };
        State m_state{};
        bool  m_flip = false;
        float m_timer = 0.f;
        float m_innerT = 0.f;
        int   m_shotsLeft = 0;
        Config m_cfg;

        void beginPuffVolley ( ) {
            m_state = State::Puffing;
            m_shotsLeft = m_cfg.puffVolleyCount;
            m_innerT = 0.f; // 즉시 1발
        }

        void firePuffOnce ( ) {
            if ( !m_spawnProjId ) return;
            // 보스 중앙에서 플레이어를 향해 직선 발사
            engine::Vec2 me = center ( );
            engine::Vec2 tp = m_queryTarget ? m_queryTarget ( ) : me;
            engine::Vec2 dir = { tp.x - me.x, tp.y - me.y };
            const float len = std::max ( 1.f , std::sqrt ( dir.x * dir.x + dir.y * dir.y ) );
            dir.x /= len; dir.y /= len;

            engine::Vec2 vel = { dir.x * m_cfg.puffSpeed, dir.y * m_cfg.puffSpeed };
            m_spawnProjId ( "AirPuff" , me , vel , ProjOwner::Enemy );
        }

        void beginAppleDrop ( ) {
            m_state = State::AppleDrop;
            // 즉시 Drop 1회; 연출을 더하고 싶으면 텔레그래프용 자식(가짜) 생성 가능
        }

        void dropApplesOnce ( ) {
            if ( !m_spawnMonster ) return;
            const engine::Vec2 me = center ( );
            // 보스 머리 위 Y(맵 위쪽)에서 일정 간격으로 N개
            const float span = m_cfg.appleSpanPx;
            const int   n = std::max ( 1 , m_cfg.applesPerWave );
            for ( int i = 0; i < n; ++i ) {
                const float t = ( n == 1 ) ? 0.f : ( ( float ) i / ( n - 1 ) - 0.5f ); // [-0.5, +0.5]
                const float x = me.x + t * span;
                const float y = me.y - 220.f; // 머리 위 적당한 높이
                SpawnSpec spec; // dir/flags는 Apple이 내부에서 결정
                spec.type = MonsterType::Apple; spec.x = x; spec.y = y; spec.dir = 0; spec.attack = 1; spec.move = 1;
                m_spawnMonster ( MonsterType::Apple , { x,y } , spec );
            }
        }

        engine::Vec2 center ( ) const {
            int x , y , w , h; GetBounds ( x , y , w , h );
            return { x + w * 0.5f, y + h * 0.5f };
        }
    };

} // namespace game
