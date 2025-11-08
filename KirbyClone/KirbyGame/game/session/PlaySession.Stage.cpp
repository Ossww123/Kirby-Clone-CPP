// PlaySession.Stage.cpp
//
// Responsibility: Stage loading and world setup (tiles/monsters/background/player/camera bounds).
// Non-Goals    : Renderer creation, gameplay rules beyond initial spawn.
// Call-Context : Called by PlaySession to (re)load a stage and rebuild systems.

#include <algorithm>
#include <cctype>

#include "game/session/PlaySession.h"
#include "engine/render/D3D11Renderer.h"
#include "engine/render/TextureLoader.h"
#include "engine/util/StringConv.h"
#include "engine/util/Types.h"
#include "engine/world/TileSet.h"                 // engine::TileDef
#include "game/data/StageCSV.h"
#include "game/data/StageDesc.h"
#include "game/entities/monsters/MonsterFactory.h"
#include "game/data/GameConfig.h"

namespace game {

    bool PlaySession::LoadStage ( const char* jsonPath ) {
        // ---- stage.json ----
        m_stageJsonPath = jsonPath ? jsonPath : m_stageJsonPath;

        game::StageDesc desc{};
        if ( !game::LoadStageDesc ( m_stageJsonPath.c_str ( ) , desc ) ) return false;

        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( !d3d ) return false;

        // ---- Tile/Map define ----
        m_World.LoadTileset ( d3d->Device ( ) , ToWide ( desc.tileset ).c_str ( ) , 16 , 16 );
        m_World.SetWorldTileSize ( game::TILE_PX , game::TILE_PX );

        int mw = 0 , mh = 0; std::vector<int> ids;
        game::LoadTileMapCSV ( desc.tilemap.c_str ( ) , mw , mh , ids );
        m_World.SetMapFromMemory ( mw , mh , ids.data ( ) );

        std::vector<game::TileDefCSV> tdefs;
        if ( game::LoadTileDefsCSV ( desc.tiledefs.c_str ( ) , tdefs ) && !tdefs.empty ( ) ) {
            const int cw = 16 , ch = 16;
            for ( const auto& r : tdefs ) {
                engine::TileDef d{};
                d.solid = ( r.solid != 0 );
                d.oneway = ( r.oneway != 0 );
                if ( r.gx >= 0 && r.gy >= 0 )
                    d.src = engine::IntRect{ r.gx * cw, r.gy * ch, r.gx * cw + cw, r.gy * ch + ch };
                m_World.DefineTile ( r.id , d );
            }
        }
        m_World.RebuildColliders ( );

        // --- Boss Arena / Camera lock ---
        m_worldRectFull = m_World.WorldRectPx ( );
        m_hasBossArena = desc.has_boss_arena;
        if ( m_hasBossArena ) {
            m_bossArena = { desc.boss_x, desc.boss_y, desc.boss_x + desc.boss_w, desc.boss_y + desc.boss_h };
        }
        else {
            m_bossArena = { 0,0,0,0 };
        }
        m_bossCamLocked = false;

        // ---- Background ----
        if ( !desc.background.empty ( ) ) {
            engine::Tex2D bg{};
            if ( engine::LoadTextureWIC ( d3d->Device ( ) , ToWide ( desc.background ).c_str ( ) , &bg ) ) {
                m_BgTex = bg;
            }
            else {
                m_BgTex = {};
            }
        }
        else {
            m_BgTex = {};
        }

        // ---- Player start ----
        game::PlayerStartCSV ps{};
        if ( game::LoadPlayerStartCSV ( desc.player_start.c_str ( ) , ps ) && m_Player ) {
            m_Player->SetPosition ( ps.x , ps.y );
            m_Player->Body ( ).SetVelocity ( { 0.f , 0.f } );
            updateCameraBoundsForWorld (
                m_Renderer ? m_Renderer->GetBackbufferSize ( ).w : 0 ,
                m_Renderer ? m_Renderer->GetBackbufferSize ( ).h : 0
            );
            m_Cam.SetLookAt ( { ps.x , ps.y } );
            m_Cam.SnapImmediate ( );
        }

        // ---- Load monsters ----
        m_Monsters.clear ( );
        if ( !desc.monsters.empty ( ) ) {
            std::vector<game::MonsterCSV> mons;
            if ( game::LoadMonstersCSV ( desc.monsters.c_str ( ) , mons ) ) {
                for ( auto& r : mons ) {
                    // type string → enum
                    std::string t = r.type;
                    std::transform ( t.begin ( ) , t.end ( ) , t.begin ( ) ,
                        [ ] ( unsigned char c ) { return ( char ) std::tolower ( c ); } );
                    game::MonsterType mt;
                    if ( t == "waddledee" ) mt = game::MonsterType::WaddleDee;
                    else if ( t == "waddledoo" ) mt = game::MonsterType::WaddleDoo;
                    else if ( t == "hothead" ) mt = game::MonsterType::HotHead;
                    else if ( t == "sparky" ) mt = game::MonsterType::Sparky;
                    else if ( t == "whispywoods" ) mt = game::MonsterType::WhispyWoods;
                    else if ( t == "apple" ) mt = game::MonsterType::Apple;
                    else continue;

                    game::SpawnSpec spec{};
                    spec.type = mt; spec.x = r.x; spec.y = r.y;
                    spec.dir = ( r.dir < 0 ? -1 : ( r.dir > 0 ? +1 : 0 ) );
                    spec.attack = r.attack; spec.move = r.move;

                    auto mon = game::MonsterFactory::Create ( mt , m_World.WorldRectPx ( ) , &m_World.Collision ( ) , spec );
                    if ( !mon ) continue;

                    // Texture/visual size/source
                    if ( m_EnemiesTex.srv ) mon->SetSpriteSheet ( &m_EnemiesTex );
                    mon->SetVisualSize ( 32.f , 32.f );
                    engine::IntRect src{};
                    switch ( mt ) {
                    case game::MonsterType::WaddleDee: src = { 8,   8,   40,  40 }; break;
                    case game::MonsterType::WaddleDoo: src = { 8,   40,  40,  72 }; break;
                    case game::MonsterType::HotHead:   src = { 8,   136, 40,  168 }; break;
                    case game::MonsterType::Sparky:    src = { 8,   168, 40,  200 }; break;
                    default: break;
                    }
                    mon->SetSpriteSrc ( src );

                    // WhispyWoods: large static boss collider
                    if ( mt == game::MonsterType::WhispyWoods ) {
                        mon->SetSize ( 6.f * game::TILE_PX , 8.f * game::TILE_PX ); // 96x128px
                        mon->SetKnockbackMul ( 0.f );
                        // Optionally scale visual too:
                        // mon->SetVisualSize( 6.f * game::TILE_PX, 8.f * game::TILE_PX );
                    }

                    // Monster → projectile/hitvolume spawners
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
                        auto s = spec; s.type = t; s.x = pos.x; s.y = pos.y; m_pendingMonsterSpawns.push_back ( s );
                    } );

                    m_Monsters.push_back ( std::move ( mon ) );
                }
            }
        }

        // ---- Doors ----
        m_Doors.clear ( );
        if ( !desc.doors.empty ( ) ) {
            std::vector<game::DoorCSV> tmp;
            if ( game::LoadDoorsCSV ( desc.doors.c_str ( ) , tmp ) ) m_Doors = std::move ( tmp );
        }

        // Rebuild combat systems after world/colliders/player placement
        initCombatSystems ( );
        return true;
    }

    void PlaySession::initPlayerAndCamera ( const engine::IntRect& rcClient ) {
        if ( !m_Scene ) return;
        m_Player = m_Scene->Spawn<game::Player> ( rcClient );
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) , m_Player->Animator ( ) , m_playerFsmCfg );

        const int w = rcClient.r - rcClient.l;
        const int h = rcClient.b - rcClient.t;
        m_Cam.SetScreenSize ( w , h );
        m_Cam.SetSmoothSpeed ( 10.f );
        m_Cam.SetPixelSnap ( true );
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.SnapImmediate ( );
    }

    void PlaySession::updateCameraBoundsForWorld ( int sw , int sh ) {
        engine::IntRect wr0 = m_World.WorldRectPx ( );
        if ( wr0.r <= wr0.l || wr0.b <= wr0.t ) return;

        const int viewW_world = sw;
        const int viewH_world = sh;
        const int padWorld = game::TILE_PX / 2;

        engine::IntRect wr = wr0;
        const int wldW = wr.r - wr.l , wldH = wr.b - wr.t;
        if ( wldW > viewW_world ) { wr.l += padWorld; wr.r -= padWorld; }
        if ( wldH > viewH_world ) { wr.t += padWorld; wr.b -= padWorld; }

        // If boss-locked, clamp to arena; else use padded world
        if ( m_bossCamLocked && m_hasBossArena ) {
            m_Cam.SetWorldRect ( m_bossArena );
        }
        else {
            m_Cam.SetWorldRect ( wr );
        }
    }

} // namespace game
