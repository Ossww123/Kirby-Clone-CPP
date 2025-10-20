#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include "game/Monster.h"

// --- 몬스터 ---
#include "game/WaddleDee.h"
#include "game/WaddleDoo.h"
#include "game/HotHead.h"
#include "game/Sparky.h"

namespace game {

    enum class MonsterType { WaddleDee , WaddleDoo , HotHead, Sparky /* , ... */ };

    struct SpawnSpec {
        MonsterType type;
        float x = 0.f , y = 0.f;
        int   dir = 1;
        // 필요 시 사이즈/속성 추가 가능
    };

    class MonsterFactory {
    public:
        using Maker = std::function<std::unique_ptr<Monster> ( const RECT& ,
                             const engine::physics::CollisionSystem* , const SpawnSpec& )>;

        static void Register ( MonsterType t , Maker m ) { Makers ( )[ t ] = std::move ( m ); }

        static std::unique_ptr<Monster> Create ( MonsterType t , const RECT& bounds ,
                                               const engine::physics::CollisionSystem* col ,
                                               const SpawnSpec& spec )
        {
            auto it = Makers ( ).find ( t );
            if ( it == Makers ( ).end ( ) ) return nullptr;
            auto mon = it->second ( bounds , col , spec );
            if ( mon ) mon->SetPosition ( spec.x , spec.y );
            return mon;
        }

        static void RegisterDefaults ( ) {
            // --- WaddleDee 기본 등록 ---
            Register ( MonsterType::WaddleDee ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                        WaddleDee::Config cfg;
                        cfg.dir = ( s.dir >= 0 ) ? 1 : -1;
                        // 몬스터 물리 튜닝(걷기 전용으로 살짝 완만하게)
                        cfg.base.phys.accelRun = 1400.f;
                        cfg.base.phys.decelRun = 1600.f;
                        cfg.base.phys.maxSpeedRun = 70.f;
                        cfg.base.phys.frictionGround = 500.f;
                        cfg.base.phys.frictionAir = 80.f;
                        cfg.base.phys.gravity = 1200.f;
                        cfg.base.phys.termVel = 1050.f;
                        cfg.base.ignoreOneWayUpward = false; // 웨이들디는 원웨이 위로 못 올라감

                        return std::make_unique<WaddleDee> ( b , col , cfg );
                } );

            Register ( MonsterType::WaddleDoo ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                        WaddleDoo::Config cfg;

                        // --- 이동(걷기 가능) ---
                        cfg.base.phys.accelRun = 1400.f;
                        cfg.base.phys.decelRun = 1600.f;
                        cfg.base.phys.maxSpeedRun = 65.f;   // Dee(약 70)보다 살짝 느리게
                        cfg.base.phys.frictionGround = 500.f;
                        cfg.base.phys.frictionAir = 80.f;
                        cfg.base.phys.gravity = 1200.f;
                        cfg.base.phys.termVel = 1050.f;
                        cfg.base.ignoreOneWayUpward = false;

                        // --- 이동 공통 키(Dee와 동일) ---
                        cfg.dir = ( s.dir >= 0 ) ? 1 : -1;
                        cfg.turnOnHitX = true;
                        cfg.turnAtEdge = true;

                        // --- 공격 ---
                        cfg.wakeRange = 360.f;
                        cfg.windupMs = 0.35f;
                        cfg.firePeriod = 1.20f;
                        cfg.bulletSpeed = 420.f;
                        cfg.stopDuringWindup = true;

                        return std::make_unique<WaddleDoo> ( b , col , cfg );
                } );

            // --- HotHead (Fire) ---
            Register ( MonsterType::HotHead , [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                HotHead::Config cfg;
                cfg.base.phys.accelRun = 1200.f;
                cfg.base.phys.decelRun = 1500.f;
                cfg.base.phys.maxSpeedRun = 45.f; // 느긋하게
                cfg.base.phys.frictionGround = 520.f;
                cfg.base.phys.frictionAir = 80.f;
                cfg.base.phys.gravity = 1200.f;
                cfg.base.phys.termVel = 1050.f;
                cfg.base.ignoreOneWayUpward = false;
                cfg.dir = ( s.dir >= 0 ) ? 1 : -1;
                cfg.wakeRange = 260.f;
                cfg.windupMs = 0.25f;
                cfg.breathMs = 0.55f;
                cfg.fireIntervalMs = 0.06f;
                cfg.bulletSpeed = 360.f;
                cfg.stopDuringWindup = true;
                // 쿨다운(HotHead 소스에 firePeriod 사용 시)
                // 필요하면 HotHead::Config에 float firePeriod 추가하세요.
                return std::make_unique<HotHead> ( b , col , cfg );
            } );

            // --- Sparky (Spark) ---
            Register ( MonsterType::Sparky , [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                Sparky::Config cfg;
                cfg.base.phys.accelRun = 1400.f;
                cfg.base.phys.decelRun = 1600.f;
                cfg.base.phys.maxSpeedRun = 30.f; // 아주 천천히
                cfg.base.phys.frictionGround = 520.f;
                cfg.base.phys.frictionAir = 80.f;
                cfg.base.phys.gravity = 1200.f;
                cfg.base.phys.termVel = 1050.f;
                cfg.base.ignoreOneWayUpward = false;
                cfg.dir = ( s.dir >= 0 ) ? 1 : -1;
                cfg.hopPeriodMs = 0.8f;
                cfg.hopVy = 360.f;
                cfg.wakeRange = 220.f;
                cfg.windupMs = 0.30f;
                cfg.firePeriod = 1.40f;
                cfg.ringProjectiles = 10;
                cfg.sparkSpeed = 260.f;
                cfg.stopDuringWindup = true;
                return std::make_unique<Sparky> ( b , col , cfg );
            } );
        
        }

    private:
        static std::unordered_map<MonsterType , Maker>& Makers ( ) {
            static std::unordered_map<MonsterType , Maker> s;
            return s;
        }
    };

} // namespace game
