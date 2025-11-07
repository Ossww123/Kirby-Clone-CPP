// PlaySession.Combat.cpp

#include "game/PlaySession.h"
#include "game/Monster.h"
#include "game/MonsterFactory.h"
#include "game/ProjectileFactory.h"
#include "game/HitVolumeFactory.h"

namespace game {
    void PlaySession::registerDefaultFactories ( ) {
        game::MonsterFactory::RegisterDefaults ( );
        game::ProjectileFactory::RegisterDefaults ( );
        game::HitVolumeFactory::RegisterDefaults ( );
    }

    void PlaySession::initCombatSystems ( ) {
        m_projSys.Initialize ( m_World.WorldRectPx ( ) , &m_World.Collision ( ) );
        m_hitSys.Initialize ( );
        m_hitSys.SetOwnerLocator ( [ this ] ( int ownerId , engine::Vec2& pos , int& fac ) {
            if ( m_Player && m_Player->Id ( ) == ownerId ) { pos = m_Player->Center ( ); fac = m_PlayerFSM.Facing ( ); return true; }
            for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ownerId ) { pos = m->Center ( ); fac = m->Facing ( ); return true; }
            return false;
        } );
    }

    void PlaySession::updateMonsters ( double fixedDt , const engine::Input& input ) {
        for ( auto& m : m_Monsters ) if ( m ) m->Update ( fixedDt , input );
    }

    void PlaySession::handlePlayerEvents ( const std::vector<game::PlayerEvent>& evs ) {
        for ( auto& e : evs ) switch ( e.type ) {
        case game::PlayerEvent::DoorInteract: {
            checkDoorInteract ( ); break;
        }
        case game::PlayerEvent::InhaleVolume: {
            game::HitVolumeSystem::SpawnDesc sd{ "InhaleField", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::SpitStar: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "Star"; sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f,
                       float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f }; sd.treatAsDirection = true;
            m_projSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AirPuffShot: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "AirPuff"; sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f,
                       float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f }; sd.treatAsDirection = true;
            m_projSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AbilityFire: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const float x = ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 10.f;
            const float y = float ( py + ph * 0.5f - 4.f );
            for ( int i = 0; i < 3; ++i ) {
                const float base = 360.f , jitter = 40.f * ( i - 1 );
                game::ProjectileSystem::SpawnDesc sd{};
                sd.archetype = "FirePellet"; sd.owner = game::ProjOwner::Player;
                sd.pos = { x, y };
                sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ) * ( base + jitter ), 0.f };
                sd.treatAsDirection = false; // ← 속도 벡터로 취급
                m_projSys.Spawn ( sd );
            }
            break;
        }
        case game::PlayerEvent::AbilitySpark: {
            game::HitVolumeSystem::SpawnDesc sd{ "SparkAura", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        case game::PlayerEvent::AbilityBeam: {
            game::HitVolumeSystem::SpawnDesc sd{ "BeamSweep", m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
            m_hitSys.Spawn ( sd ); break;
        }
        default: break;
        }
    }

    void PlaySession::buildTargets ( std::vector<game::ProjectileSystem::Target>& projT ,
                               std::vector<game::HitVolumeSystem::Target>& hvT ) {
        projT.clear ( ); hvT.clear ( );
        projT.reserve ( m_Monsters.size ( ) + 1 );
        hvT.reserve ( m_Monsters.size ( ) + 1 );
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            projT.push_back ( { m->Id ( ), RECT{mx,my,mx + mw,my + mh}, true, /*isPlayer*/false } );
            game::HitVolumeSystem::Target t{};
            t.id = m->Id ( ); t.aabb = RECT{ mx,my,mx + mw,my + mh }; t.alive = true; t.isPlayer = false;
            t.inhalable = m->Inhalable ( ); t.abilityGift = m->AbilityGift ( );
            hvT.push_back ( t );
        }
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            projT.push_back ( { m_Player->Id ( ), RECT{px,py,px + pw,py + ph}, true, /*isPlayer*/true } );
            game::HitVolumeSystem::Target pt{}; pt.id = m_Player->Id ( ); pt.aabb = { px,py,px + pw,py + ph };
            pt.alive = true; pt.isPlayer = true; pt.inhalable = false; pt.abilityGift = game::Ability::None;
            hvT.push_back ( pt );
        }
    }

    void PlaySession::applyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>& phits ) {
        for ( const auto& ev : phits ) {
            game::Damage dmg{ ev.payload.damage, ev.payload.knockback };
            if ( ev.owner == game::ProjOwner::Player ) {
                for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ev.targetId ) { m->OnHit ( dmg ); break; }
            }
            else {
                if ( m_Player && m_Player->Id ( ) == ev.targetId ) m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void PlaySession::applyHitVolumeHits ( const std::vector<game::HitVolumeSystem::HitEvent>& hvHits ) {
        for ( const auto& ev : hvHits ) {
            const bool ownerIsPlayer = ( m_Player && ev.ownerId == m_Player->Id ( ) );
            if ( ev.payload.effect == game::HitEffect::Capture ) { // 빨아들이기 캡쳐
                if ( ownerIsPlayer ) {
                    for ( auto it = m_Monsters.begin ( ); it != m_Monsters.end ( ); ++it ) {
                        if ( *it && ( *it )->Id ( ) == ev.targetId ) {
                            const game::Ability gift = ( ev.gift != game::Ability::None ) ? ev.gift : game::Ability::None;
                            m_Monsters.erase ( it );
                            m_PlayerFSM.OnMouthCatch ( gift );
                            break;
                        }
                    }
                }
                continue;
            }
            game::Damage dmg{ ev.payload.damage, ev.payload.knockback };
            if ( ownerIsPlayer ) {
                for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ev.targetId ) { m->OnHit ( dmg ); break; }
            }
            else {
                if ( m_Player && m_Player->Id ( ) == ev.targetId ) m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void PlaySession::flushPendingSpawns ( ) {
        if ( m_pendingMonsterSpawns.empty ( ) ) return;
        auto spawns = std::move ( m_pendingMonsterSpawns );
        for ( const auto& s : spawns ) {
            auto mon = game::MonsterFactory::Create ( s.type , m_World.WorldRectPx ( ) , &m_World.Collision ( ) , s );
            if ( !mon ) continue;
            if ( m_EnemiesTex.srv ) mon->SetSpriteSheet ( &m_EnemiesTex );
            mon->SetVisualSize ( 32.f , 32.f );

            // ※ 런타임 스폰도 콜백을 동일하게 접속해야 보스(위스피)→사과 드랍/공기포가 실제 동작
            mon->SetProjectileSpawnerId ( [ this ] ( const std::string& arche , const engine::Vec2& pos ,
                const engine::Vec2& vel , game::ProjOwner owner ) {
                    game::ProjectileSystem::SpawnDesc sd{}; sd.archetype = arche; sd.owner = owner;
                    sd.pos = pos; sd.dirOrVel = vel; sd.treatAsDirection = false; m_projSys.Spawn ( sd );
            } );
            mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );
            mon->SetHitVolumeSpawner ( [ this ] ( const std::string& arche , int ownerId , int facing , const engine::Vec2& anchor ) {
                game::HitVolumeSystem::SpawnDesc sd{ arche, ownerId, facing, anchor }; m_hitSys.Spawn ( sd );
            } );
            mon->SetMonsterSpawner ( [ this ] ( MonsterType t , const engine::Vec2& pos , const SpawnSpec& spec ) {
                auto s2 = spec; s2.type = t; s2.x = pos.x; s2.y = pos.y; m_pendingMonsterSpawns.push_back ( s2 );
            } );

            m_Monsters.push_back ( std::move ( mon ) );
        }
    }
} // namespace game
