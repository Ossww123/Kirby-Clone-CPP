#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include "game/Monster.h"
#include "game/MonsterTypes.h"

// --- 몬스터 ---
#include "game/WaddleDee.h"
#include "game/WaddleDoo.h"
#include "game/HotHead.h"
#include "game/Sparky.h"
#include "game/Apple.h"
#include "game/WhispyWoods.h"

namespace game {
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
                    // fixed
                    cfg.base.phys.accelRun = 1400.f; cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 70.f; cfg.base.phys.frictionGround = 500.f;
                    cfg.base.phys.frictionAir = 80.f; cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f;  cfg.base.ignoreOneWayUpward = false;

                    // instance flag
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    cfg.enableAttack = ( s.attack != 0 );
                    cfg.enableMove = ( s.move != 0 );
                    return std::make_unique<WaddleDee> ( b , col , cfg );
                } );

            // --- WaddleDoo ---
            Register ( MonsterType::WaddleDoo ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    WaddleDoo::Config cfg;
                    // fixed
                    cfg.base.phys.accelRun = 1400.f; cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 65.f; cfg.base.phys.frictionGround = 500.f;
                    cfg.base.phys.frictionAir = 80.f; cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f; cfg.base.ignoreOneWayUpward = false;
                    cfg.turnOnHitX = true; cfg.turnAtEdge = true;
                    cfg.wakeRange = 360.f; cfg.windupMs = 0.35f; cfg.firePeriod = 1.20f;
                    cfg.stopDuringWindup = true;
                    
                    // instance flag
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    cfg.enableAttack = ( s.attack != 0 );
                    cfg.enableMove = ( s.move != 0 );
                    return std::make_unique<WaddleDoo> ( b , col , cfg );
                } );

            // --- HotHead ---
            Register ( MonsterType::HotHead ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    HotHead::Config cfg;
                    // 타입 고정 튜닝
                    cfg.base.phys.accelRun = 1200.f;
                    cfg.base.phys.decelRun = 1500.f;
                    cfg.base.phys.maxSpeedRun = 45.f;
                    cfg.base.phys.frictionGround = 520.f;
                    cfg.base.phys.frictionAir = 80.f;
                    cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f;
                    cfg.base.ignoreOneWayUpward = false;

                    cfg.turnOnHitX = true; cfg.turnAtEdge = true;
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );

                    cfg.wakeRange = 260.f;
                    cfg.windupMs = 0.25f;
                    cfg.breathMs = 0.55f;
                    cfg.fireIntervalMs = 0.06f;
                    cfg.bulletSpeed = 360.f;
                    cfg.stopDuringWindup = true;
                    cfg.firePeriod = 1.10f;

                    // 인스턴스 플래그
                    cfg.enableMove = ( s.move != 0 );
                    cfg.enableAttack = ( s.attack != 0 );
                    return std::make_unique<HotHead> ( b , col , cfg );
                } );

            // --- Sparky ---
            Register ( MonsterType::Sparky ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    Sparky::Config cfg;
                    // 타입 고정 튜닝
                    cfg.base.phys.accelRun = 1400.f;
                    cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 30.f;
                    cfg.base.phys.frictionGround = 520.f;
                    cfg.base.phys.frictionAir = 80.f;
                    cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f;
                    cfg.base.ignoreOneWayUpward = false;
                    
                    cfg.turnOnHitX = true;
                    cfg.turnAtEdge = true;
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    
                    cfg.hopRestMs = 0.5f;
                    cfg.hopVy = 360.f;
                    cfg.hopSmallDist = 32.f;
                    cfg.hopMediumDist = 96.f;
                    
                    cfg.wakeRange = 220.f;
                    cfg.windupMs = 0.30f;
                    cfg.firePeriod = 1.40f;
                    cfg.stopDuringWindup = true;
                    
                    // 인스턴스 플래그
                    cfg.enableMove = ( s.move != 0 );
                    cfg.enableAttack = ( s.attack != 0 );
                    return std::make_unique<Sparky> ( b , col , cfg );
                } );

            // --- Apple (보스 드랍 오브젝트) ---
            Register ( MonsterType::Apple ,
              [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                             Apple::Config cfg;
                             cfg.base.phys.accelRun = 2600.f;
                             cfg.base.phys.decelRun = 2600.f;
                             cfg.base.phys.maxSpeedRun = 120.f;
                             cfg.base.phys.frictionGround = 650.f;
                             cfg.base.phys.frictionAir = 40.f;
                             cfg.base.phys.gravity = 1300.f;
                             cfg.base.phys.termVel = 1100.f;
                             cfg.base.ignoreOneWayUpward = false;
                             cfg.telegraphMs = 0.6f;
                             cfg.bounceVx = 140.f;
                             cfg.bounceVy = 360.f;
                             cfg.rollSpeed = 90.f;
                             return std::make_unique<Apple> ( b , col , cfg , s.x , s.y );
              } );

            // --- WhispyWoods (보스) ---
            Register ( MonsterType::WhispyWoods ,
                [ ] ( const RECT& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                        WhispyWoods::Config cfg;
                        // 고정형: 이동속도 0, 체력/아이프레임 상향
                        cfg.base.phys.accelRun = 0.f; cfg.base.phys.decelRun = 0.f; cfg.base.phys.maxSpeedRun = 0.f;
                        cfg.base.phys.frictionGround = 0.f; cfg.base.phys.frictionAir = 0.f; cfg.base.phys.gravity = 0.f; cfg.base.phys.termVel = 0.f;
                        cfg.base.maxHp = 12; cfg.base.iFrameMs = 0.4f;
                        cfg.puffVolleyCount = 3; cfg.puffIntervalMs = 0.33f; cfg.puffSpeed = 220.f; cfg.puffRestMs = 1.4f;
                        cfg.appleRestMs = 2.8f; cfg.appleTelegraphMs = 0.65f; cfg.applesPerWave = 3; cfg.appleSpanPx = 240.f;
                        return std::make_unique<WhispyWoods> ( b , col , cfg , /*x*/s.x , /*y*/s.y );
                } );


        }

    private:
        static std::unordered_map<MonsterType , Maker>& Makers ( ) {
            static std::unordered_map<MonsterType , Maker> s;
            return s;
        }
    };

} // namespace game
