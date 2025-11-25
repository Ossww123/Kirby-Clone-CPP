// PlaySession.Combat.cpp
//
// Responsibility: Combat systems init, target building, player event handling,
//                 damage application, and runtime monster spawns.
// Non-Goals    : Rendering, asset policy.
// Call-Context : Called by PlaySession during init and fixed update.

#include "game/session/PlaySession.h"
#include "game/entities/monsters/Monster.h"
#include "game/entities/monsters/MonsterFactory.h"
#include "game/entities/player/Player.h"
#include "game/projectile/ProjectileFactory.h"
#include "game/combat/HitVolumeFactory.h"
#include "game/combat/Damage.h"
#include "engine/platform/win32/RectUtil.h"

#include <algorithm>

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
        case game::PlayerEvent::DoorInteract:
            checkDoorInteract ( );
            break;

        case game::PlayerEvent::InhaleVolume: {
            constexpr const char* k = "InhaleField";
            if ( m_playerHVActive.find ( k ) == m_playerHVActive.end ( ) ) {
                game::HitVolumeSystem::SpawnDesc sd{ k, m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
                if ( int id = m_hitSys.Spawn ( sd ); id > 0 ) m_playerHVActive.emplace ( k , id );
            }
            break;
        }

        case game::PlayerEvent::SpitStar: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "Star";
            sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f,
                             float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f };
            sd.treatAsDirection = true;
            m_projSys.Spawn ( sd );
            break;
        }

        case game::PlayerEvent::AirPuffShot: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            game::ProjectileSystem::SpawnDesc sd{};
            sd.archetype = "AirPuff";
            sd.owner = game::ProjOwner::Player;
            sd.pos = { ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 8.f,
                             float ( py + ph * 0.5f - 4.f ) };
            sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ), 0.f };
            sd.treatAsDirection = true;
            m_projSys.Spawn ( sd );
            break;
        }

        case game::PlayerEvent::AbilityFire: {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const float x = ( m_PlayerFSM.Facing ( ) > 0 ) ? float ( px + pw ) : float ( px ) - 10.f;
            const float y = float ( py + ph * 0.5f - 4.f );
            for ( int i = 0; i < 3; ++i ) {
                const float base = 360.f , jitter = 40.f * ( i - 1 );
                game::ProjectileSystem::SpawnDesc sd{};
                sd.archetype = "FirePellet";
                sd.owner = game::ProjOwner::Player;
                sd.pos = { x , y };
                sd.dirOrVel = { float ( m_PlayerFSM.Facing ( ) ) * ( base + jitter ) , 0.f };
                sd.treatAsDirection = false; // use as velocity
                m_projSys.Spawn ( sd );
            }
            break;
        }

        case game::PlayerEvent::AbilitySpark: {
            constexpr const char* k = "SparkAura";
            if ( m_playerHVActive.find ( k ) == m_playerHVActive.end ( ) ) {
                game::HitVolumeSystem::SpawnDesc sd{ k, m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
                if ( int id = m_hitSys.Spawn ( sd ); id > 0 ) m_playerHVActive.emplace ( k , id );
            }
            break;
        }

        case game::PlayerEvent::AbilityBeam: {
            constexpr const char* k = "BeamSweep";
            if ( m_playerHVActive.find ( k ) == m_playerHVActive.end ( ) ) {
                game::HitVolumeSystem::SpawnDesc sd{ k, m_Player->Id ( ), m_PlayerFSM.Facing ( ), m_Player->Center ( ) };
                if ( int id = m_hitSys.Spawn ( sd ); id > 0 ) m_playerHVActive.emplace ( k , id );
            }
            break;
        }

        case PlayerEvent::Died:
            // 지금은 1P만 있으니 P1으로 고정.
            // 나중에 2P를 추가하면: 각 PlayerFSM에서 이벤트를 받을 때
            // "어느 슬롯의 FSM인지"에 따라 PlayerSlot::P1 / P2를 넘기면 된다.
            onPlayerDied ( PlayerSlot::P1 );
            break;

        default: break;
        }
    }

    void PlaySession::buildTargets ( std::vector<game::ProjectileSystem::Target>& projT ,
                                     std::vector<game::HitVolumeSystem::Target>& hvT ) {
        projT.clear ( ); hvT.clear ( );
        projT.reserve ( m_Monsters.size ( ) + 1 );
        hvT.reserve ( m_Monsters.size ( ) + 1 );

        // Monsters
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );

            // Projectile target
            game::ProjectileSystem::Target pt{};
            pt.id = m->Id ( );
            pt.aabb = engine::IntRect{ mx , my , mx + mw , my + mh };
            pt.alive = true;
            pt.isPlayer = false;
            projT.push_back ( pt );

            // HitVolume target
            game::HitVolumeSystem::Target ht{};
            ht.id = m->Id ( );
            ht.aabb = engine::IntRect{ mx , my , mx + mw , my + mh };
            ht.alive = true;
            ht.isPlayer = false;
            ht.inhalable = m->Inhalable ( );
            ht.abilityGift = m->AbilityGift ( );
            hvT.push_back ( ht );
        }

        // Player
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );

            game::ProjectileSystem::Target pt{};
            pt.id = m_Player->Id ( );
            pt.aabb = engine::IntRect{ px , py , px + pw , py + ph };
            pt.alive = true;
            pt.isPlayer = true;
            projT.push_back ( pt );

            game::HitVolumeSystem::Target ht{};
            ht.id = m_Player->Id ( );
            ht.aabb = engine::IntRect{ px , py , px + pw , py + ph };
            ht.alive = true;
            ht.isPlayer = true;
            ht.inhalable = false;
            ht.abilityGift = game::Ability::None;
            hvT.push_back ( ht );
        }
    }

    void PlaySession::applyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>& phits ) {
        for ( const auto& ev : phits ) {
            game::Damage dmg{ ev.payload.damage , ev.payload.knockback };
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

            // Capture (inhale)
            if ( ev.payload.effect == game::HitEffect::Capture ) {
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

            // Normal damage
            game::Damage dmg{ ev.payload.damage , ev.payload.knockback };
            if ( ownerIsPlayer ) {
                for ( auto& m : m_Monsters ) if ( m && m->Id ( ) == ev.targetId ) { m->OnHit ( dmg ); break; }
            }
            else {
                if ( m_Player && m_Player->Id ( ) == ev.targetId ) m_PlayerFSM.ApplyDamage ( dmg );
            }
        }
    }

    void PlaySession::handleHitVolumeDespawns ( const std::vector<game::HitVolumeSystem::DespawnEvent>& devs ) {
        if ( m_playerHVActive.empty ( ) ) return;
        for ( const auto& ev : devs ) {
            for ( auto it = m_playerHVActive.begin ( ); it != m_playerHVActive.end ( ); ) {
                if ( it->second == ev.volumeId ) it = m_playerHVActive.erase ( it );
                else ++it;
            }
        }
    }

    void PlaySession::flushPendingSpawns ( ) {
        if ( m_pendingMonsterSpawns.empty ( ) ) return;

        auto spawns = std::move ( m_pendingMonsterSpawns );
        for ( const auto& s : spawns ) {
            auto mon = game::MonsterFactory::Create ( s.type , m_World.WorldRectPx ( ) , &m_World.Collision ( ) , s );
            if ( !mon ) continue;

            if ( m_EnemiesTex.srv ) mon->SetTexture ( &m_EnemiesTex );
            mon->SetVisualSize ( 32.f , 32.f );

            // Connect the same spawners for runtime monsters
            mon->SetProjectileSpawnerId ( [ this ] ( const std::string& arche , const engine::Vec2& pos ,
                const engine::Vec2& vel , game::ProjOwner owner ) {
                    game::ProjectileSystem::SpawnDesc sd{};
                    sd.archetype = arche; sd.owner = owner; sd.pos = pos; sd.dirOrVel = vel; sd.treatAsDirection = false;
                    m_projSys.Spawn ( sd );
            } );
            mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );
            mon->SetHitVolumeSpawner ( [ this ] ( const std::string& arche , int ownerId , int facing , const engine::Vec2& anchor ) {
                game::HitVolumeSystem::SpawnDesc sd{ arche , ownerId , facing , anchor };
                m_hitSys.Spawn ( sd );
            } );
            mon->SetMonsterSpawner ( [ this ] ( MonsterType t , const engine::Vec2& pos , const SpawnSpec& spec ) {
                auto s2 = spec; s2.type = t; s2.x = pos.x; s2.y = pos.y; m_pendingMonsterSpawns.push_back ( s2 );
            } );

            m_Monsters.push_back ( std::move ( mon ) );
        }
    }

} // namespace game
