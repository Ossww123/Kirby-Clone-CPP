#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include "game/Monster.h"
#include "game/WaddleDee.h"

namespace game {

    enum class MonsterType { WaddleDee /*, WaddleDoo, HotHead, Sparky, ... */ };

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
                        WaddleDee::DeeCfg cfg;
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
        }

    private:
        static std::unordered_map<MonsterType , Maker>& Makers ( ) {
            static std::unordered_map<MonsterType , Maker> s;
            return s;
        }
    };

} // namespace game
