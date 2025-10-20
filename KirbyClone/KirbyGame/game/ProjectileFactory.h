#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "game/Projectile.h"          // 기존 Projectile
#include "engine/Collision.h"

namespace game {

    struct ProjDef {
        float width = 8.f , height = 8.f;
        float speed = 480.f , ttl = 1.5f;
        float gravity = 0.f , frictionAir = 0.f , frictionGround = 0.f , termVel = 99999.f;
        bool  dieOnAnyWorldHit = true , ignoreOneWay = true;
        int   damage = 1;
        engine::Vec2 knockback{ 0.f,0.f };   // 명중 시
        // 렌더링/애니가 필요하면 텍스처/클립 이름 등도 추가 가능
    };

    class ProjectileFactory {
    public:
        static void Register ( const std::string& id , const ProjDef& d );
        static void RegisterDefaults ( ); // "Star", "AirPuff", "Beam" 등
        static std::unique_ptr<Projectile> Create (
            const std::string& id ,
            const RECT& worldRect ,
            const engine::physics::CollisionSystem* col ,
            ProjOwner owner
        );

        // 선택: CSV 로딩 지원
        static bool LoadCSV ( const char* filename );

    private:
        static std::unordered_map<std::string , ProjDef>& Registry ( );
    };

} // namespace game
