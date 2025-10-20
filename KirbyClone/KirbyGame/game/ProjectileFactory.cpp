// game/ProjectileFactory.cpp
#include "game/ProjectileFactory.h"
#include <fstream>
#include <sstream>

namespace game {

    std::unordered_map<std::string , ProjDef>& ProjectileFactory::Registry ( ) {
        static std::unordered_map<std::string , ProjDef> R;
        return R;
    }

    void ProjectileFactory::Register ( const std::string& id , const ProjDef& d ) {
        Registry ( )[ id ] = d;
    }

    void ProjectileFactory::RegisterDefaults ( ) {
        Register ( "Star" , ProjDef{
            .width = 8, .height = 8, .speed = 620.f, .ttl = 1.5f,
            .gravity = 0.f, .frictionAir = 0.f, .frictionGround = 0.f, .termVel = 99999.f,
            .dieOnAnyWorldHit = true, .ignoreOneWay = true,
            .damage = 1, .knockback = {300.f,-120.f}
        } );
        Register ( "AirPuff" , ProjDef{
            .width = 8, .height = 8, .speed = 420.f, .ttl = 0.6f,
            .gravity = 0.f, .frictionAir = 0.f, .frictionGround = 0.f, .termVel = 99999.f,
            .dieOnAnyWorldHit = true, .ignoreOneWay = true,
            .damage = 1, .knockback = {120.f,-60.f}
        } );
        // 이 후 필요시 추가
    }

    std::unique_ptr<Projectile> ProjectileFactory::Create (
        const std::string& id ,
        const RECT& worldRect ,
        const engine::physics::CollisionSystem* col ,
        ProjOwner owner )
    {
        auto it = Registry ( ).find ( id );
        if ( it == Registry ( ).end ( ) ) return {};
        const ProjDef& d = it->second;

        Projectile::Cfg cfg;
        cfg.width = d.width; cfg.height = d.height;
        cfg.speed = d.speed; cfg.ttl = d.ttl;
        cfg.dieOnAnyWorldHit = d.dieOnAnyWorldHit;
        cfg.gravity = d.gravity;
        cfg.frictionAir = d.frictionAir;
        cfg.frictionGround = d.frictionGround;
        cfg.termVel = d.termVel;
        cfg.ignoreOneWay = d.ignoreOneWay;

        auto p = std::make_unique<Projectile> ( worldRect , col , owner , cfg );
        // 필요하면 Projectile에 damage/knockback 세터 추가해서 넘겨주세요.
        return p;
    }

    // (선택) CSV 로더는 나중에 추가
    bool ProjectileFactory::LoadCSV ( const char* ) { return false; }

} // namespace game
