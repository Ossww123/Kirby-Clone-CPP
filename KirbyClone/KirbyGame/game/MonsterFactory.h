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
        MonsterType type = MonsterType::WaddleDee;
        float x = 0.f , y = 0.f;
        int   dir = 1;

        // --- CSV 오버라이드(미기재 시 -1 / 빈칸 처리) ---
        int   turnOnHitX = -1;
        int   turnAtEdge = -1;
        int   stopDuringWindup = -1;

        float wakeRange = -1.f;
        float windupMs = -1.f;
        float firePeriod = -1.f;
        float bulletSpeed = -1.f; // WaddleDoo/HotHead 용. Sparky는 있으면 sparkSpeed로 사용

        /* 필요시 속성 추가 */
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

            // --- WaddleDoo ---
            Register ( MonsterType::WaddleDoo ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                                WaddleDoo::Config cfg;
                                // 기본값
                                cfg.base.phys.accelRun = 1400.f;
                                cfg.base.phys.decelRun = 1600.f;
                                cfg.base.phys.maxSpeedRun = 65.f;
                                cfg.base.phys.frictionGround = 500.f;
                                cfg.base.phys.frictionAir = 80.f;
                                cfg.base.phys.gravity = 1200.f;
                                cfg.base.phys.termVel = 1050.f;
                                cfg.base.ignoreOneWayUpward = false;
                                cfg.dir = ( s.dir >= 0 ) ? 1 : -1;
                                cfg.turnOnHitX = true;
                                cfg.turnAtEdge = true;
                                cfg.wakeRange = 360.f;
                                cfg.windupMs = 0.35f;
                                cfg.firePeriod = 1.20f;
                                cfg.bulletSpeed = 420.f;
                                cfg.stopDuringWindup = true;

                                // CSV 오버라이드
                                if ( s.turnOnHitX >= 0 )       cfg.turnOnHitX = ( s.turnOnHitX != 0 );
                                if ( s.turnAtEdge >= 0 )       cfg.turnAtEdge = ( s.turnAtEdge != 0 );
                                if ( s.wakeRange >= 0.f )     cfg.wakeRange = s.wakeRange;
                                if ( s.windupMs >= 0.f )     cfg.windupMs = s.windupMs;
                                if ( s.firePeriod >= 0.f )     cfg.firePeriod = s.firePeriod;
                                if ( s.bulletSpeed >= 0.f )     cfg.bulletSpeed = s.bulletSpeed;
                                if ( s.stopDuringWindup >= 0 ) cfg.stopDuringWindup = ( s.stopDuringWindup != 0 );

                                return std::make_unique<WaddleDoo> ( b , col , cfg );
                } );

            // --- HotHead ---
            Register ( MonsterType::HotHead ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                                HotHead::Config cfg;
                                cfg.base.phys.accelRun = 1200.f;
                                cfg.base.phys.decelRun = 1500.f;
                                cfg.base.phys.maxSpeedRun = 45.f;
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

                                // CSV 오버라이드
                                if ( s.turnOnHitX >= 0 )       cfg.turnOnHitX = ( s.turnOnHitX != 0 );
                                if ( s.turnAtEdge >= 0 )       cfg.turnAtEdge = ( s.turnAtEdge != 0 );
                                if ( s.wakeRange >= 0.f )     cfg.wakeRange = s.wakeRange;
                                if ( s.windupMs >= 0.f )     cfg.windupMs = s.windupMs;
                                if ( s.firePeriod >= 0.f )     cfg.firePeriod = s.firePeriod;
                                if ( s.bulletSpeed >= 0.f )     cfg.bulletSpeed = s.bulletSpeed;
                                if ( s.stopDuringWindup >= 0 ) cfg.stopDuringWindup = ( s.stopDuringWindup != 0 );

                                return std::make_unique<HotHead> ( b , col , cfg );
                } );

            // --- Sparky ---
            Register ( MonsterType::Sparky ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                                Sparky::Config cfg;
                                cfg.base.phys.accelRun = 1400.f;
                                cfg.base.phys.decelRun = 1600.f;
                                cfg.base.phys.maxSpeedRun = 30.f;
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

                                // CSV 오버라이드 (bulletSpeed가 있으면 sparkSpeed로 사용)
                                if ( s.turnOnHitX >= 0 )       cfg.turnOnHitX = ( s.turnOnHitX != 0 );
                                if ( s.turnAtEdge >= 0 )       cfg.turnAtEdge = ( s.turnAtEdge != 0 );
                                if ( s.wakeRange >= 0.f )     cfg.wakeRange = s.wakeRange;
                                if ( s.windupMs >= 0.f )     cfg.windupMs = s.windupMs;
                                if ( s.firePeriod >= 0.f )     cfg.firePeriod = s.firePeriod;
                                if ( s.bulletSpeed >= 0.f )     cfg.sparkSpeed = s.bulletSpeed; // CSV 호환
                                if ( s.stopDuringWindup >= 0 ) cfg.stopDuringWindup = ( s.stopDuringWindup != 0 );

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
