#include "game/entities/monsters/MonsterFactory.h"

// Keep heavy deps out of the header:
#include "game/entities/monsters/MonsterTypes.h"
#include "game/entities/monsters/WaddleDee.h"
#include "game/entities/monsters/WaddleDoo.h"
#include "game/entities/monsters/HotHead.h"
#include "game/entities/monsters/Sparky.h"
#include "game/entities/monsters/Apple.h"
#include "game/entities/monsters/WhispyWoods.h"

namespace game {

    std::unordered_map<MonsterType , MonsterFactory::Maker>& MonsterFactory::Makers ( ) {
        static std::unordered_map<MonsterType , Maker> s;
        return s;
    }

    void MonsterFactory::Register ( MonsterType t , Maker m ) {
        Makers ( )[ t ] = std::move ( m );
    }

    std::unique_ptr<Monster> MonsterFactory::Create ( MonsterType t ,
                                                    const engine::IntRect& bounds ,
                                                    const engine::physics::CollisionSystem* col ,
                                                    const SpawnSpec& spec ) {
        auto it = Makers ( ).find ( t );
        if ( it == Makers ( ).end ( ) ) return nullptr;
        auto mon = it->second ( bounds , col , spec );
        if ( mon ) mon->SetPosition ( spec.x , spec.y );
        return mon;
    }

    void MonsterFactory::RegisterDefaults ( ) {
        // --- WaddleDee ---
        Register ( MonsterType::WaddleDee ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    WaddleDee::Cfg cfg;
                    cfg.base.phys.accelRun = 1400.f; cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 70.f; cfg.base.phys.frictionGround = 500.f;
                    cfg.base.phys.frictionAir = 80.f; cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f;  cfg.base.ignoreOneWayUpward = false;
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    cfg.enableAttack = ( s.attack != 0 );
                    cfg.enableMove = ( s.move != 0 );
                    return std::make_unique<WaddleDee> ( b , col , cfg );
            } );

        // --- WaddleDoo ---
        Register ( MonsterType::WaddleDoo ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    WaddleDoo::Cfg cfg;
                    cfg.base.phys.accelRun = 1400.f; cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 65.f; cfg.base.phys.frictionGround = 500.f;
                    cfg.base.phys.frictionAir = 80.f; cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f; cfg.base.ignoreOneWayUpward = false;
                    cfg.turnOnHitX = true; cfg.turnAtEdge = true;
                    cfg.wakeRange = 360.f; cfg.windupMs = 0.35f; cfg.firePeriod = 1.20f;
                    cfg.stopDuringWindup = true;
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    cfg.enableAttack = ( s.attack != 0 );
                    cfg.enableMove = ( s.move != 0 );
                    return std::make_unique<WaddleDoo> ( b , col , cfg );
            } );

        // --- HotHead ---
        Register ( MonsterType::HotHead ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    HotHead::Cfg cfg;
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
                    cfg.wakeRange = 260.f; cfg.windupMs = 0.25f; cfg.breathMs = 0.55f;
                    cfg.fireIntervalMs = 0.06f; cfg.bulletSpeed = 360.f;
                    cfg.stopDuringWindup = true; cfg.firePeriod = 1.10f;
                    cfg.enableMove = ( s.move != 0 );
                    cfg.enableAttack = ( s.attack != 0 );
                    return std::make_unique<HotHead> ( b , col , cfg );
            } );

        // --- Sparky ---
        Register ( MonsterType::Sparky ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    Sparky::Cfg cfg;
                    cfg.base.phys.accelRun = 1400.f;
                    cfg.base.phys.decelRun = 1600.f;
                    cfg.base.phys.maxSpeedRun = 30.f;
                    cfg.base.phys.frictionGround = 520.f;
                    cfg.base.phys.frictionAir = 80.f;
                    cfg.base.phys.gravity = 1200.f;
                    cfg.base.phys.termVel = 1050.f;
                    cfg.base.ignoreOneWayUpward = false;
                    cfg.turnOnHitX = true; cfg.turnAtEdge = true;
                    cfg.dir = ( s.dir > 0 ) ? +1 : ( s.dir < 0 ? -1 : +1 );
                    cfg.hopRestMs = 0.5f; cfg.hopVy = 360.f; cfg.hopSmallDist = 32.f; cfg.hopMediumDist = 96.f;
                    cfg.wakeRange = 220.f; cfg.windupMs = 0.30f; cfg.firePeriod = 1.40f; cfg.stopDuringWindup = true;
                    cfg.enableMove = ( s.move != 0 );
                    cfg.enableAttack = ( s.attack != 0 );
                    return std::make_unique<Sparky> ( b , col , cfg );
            } );

        // --- Apple (boss drop) ---
        Register ( MonsterType::Apple ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    Apple::Cfg cfg;
                    cfg.base.phys.accelRun = 2600.f; cfg.base.phys.decelRun = 2600.f;
                    cfg.base.phys.maxSpeedRun = 120.f;
                    cfg.base.phys.frictionGround = 650.f; cfg.base.phys.frictionAir = 40.f;
                    cfg.base.phys.gravity = 1300.f; cfg.base.phys.termVel = 1100.f;
                    cfg.base.ignoreOneWayUpward = false;
                    cfg.telegraphMs = 0.6f; cfg.bounceVx = 140.f; cfg.bounceVy = 360.f; cfg.rollSpeed = 90.f;
                    return std::make_unique<Apple> ( b , col , cfg , s.x , s.y );
            } );

        // --- WhispyWoods (boss) ---
        Register ( MonsterType::WhispyWoods ,
            [ ] ( const engine::IntRect& b , const engine::physics::CollisionSystem* col , const SpawnSpec& s ) {
                    WhispyWoods::Cfg cfg;
                    cfg.base.phys.accelRun = 0.f; cfg.base.phys.decelRun = 0.f; cfg.base.phys.maxSpeedRun = 0.f;
                    cfg.base.phys.frictionGround = 0.f; cfg.base.phys.frictionAir = 0.f;
                    cfg.base.phys.gravity = 0.f; cfg.base.phys.termVel = 0.f;
                    cfg.base.maxHp = 12; cfg.base.iFrameMs = 0.4f;
                    cfg.puffVolleyCount = 3; cfg.puffIntervalMs = 0.33f; cfg.puffSpeed = 220.f; cfg.puffRestMs = 1.4f;
                    cfg.appleRestMs = 2.8f; cfg.appleTelegraphMs = 0.65f; cfg.applesPerWave = 3; cfg.appleSpanPx = 240.f;
                    return std::make_unique<WhispyWoods> ( b , col , cfg , s.x , s.y );
            } );
    }

} // namespace game
