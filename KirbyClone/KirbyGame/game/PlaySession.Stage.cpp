// PlaySession.Stage.cpp

#include "game/PlaySession.h"
#include "engine/D3D11Renderer.h"
#include "engine/TextureLoader.h"
#include "engine/StringConv.h"
#include "game/StageCSV.h"
#include "game/StageDesc.h"
#include "game/MonsterFactory.h"
#include "game/GameConfig.h"
#include <algorithm>
#include <cctype>

namespace game {
    bool PlaySession::LoadStage ( const char* jsonPath ) {
        // ---- stage.json ---- (기존 GameApp::LoadStage 이식: 월드/배경/플레이어 시작/카메라 경계 부분)
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
            for ( auto& r : tdefs ) {
                engine::TileDef d{};
                d.solid = ( r.solid != 0 );
                d.oneway = ( r.oneway != 0 );
                if ( r.gx >= 0 && r.gy >= 0 )
                    d.src = RECT{ r.gx * cw, r.gy * ch, r.gx * cw + cw, r.gy * ch + ch };
                m_World.DefineTile ( r.id , d );
            }
        }
        m_World.RebuildColliders ( );

        // --- Boss Arena / 카메라 락 초기화 ---
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
            m_Player->Body ( ).SetVelocity ( { 0.f, 0.f } );
            updateCameraBoundsForWorld (
                m_Renderer ? m_Renderer->GetBackbufferSize ( ).w : 0 ,
                m_Renderer ? m_Renderer->GetBackbufferSize ( ).h : 0
            );
            m_Cam.SetLookAt ( { ps.x, ps.y } );
            m_Cam.SnapImmediate ( );
        }

        // ---- Load monsters ----
        m_Monsters.clear ( );
        if ( !desc.monsters.empty ( ) ) {
            std::vector<game::MonsterCSV> mons;
            if ( game::LoadMonstersCSV ( desc.monsters.c_str ( ) , mons ) ) {
                for ( auto& r : mons ) {
                    // type 문자열 → enum
                    std::string t = r.type; std::transform ( t.begin ( ) , t.end ( ) , t.begin ( ) ,
                        [ ] ( unsigned char c ) { return ( char ) std::tolower ( c ); } );
                    game::MonsterType mt;
                    if ( t == "waddledee" ) mt = game::MonsterType::WaddleDee;
                    else if ( t == "waddledoo" ) mt = game::MonsterType::WaddleDoo;
                    else if ( t == "hothead" )   mt = game::MonsterType::HotHead;
                    else if ( t == "sparky" )    mt = game::MonsterType::Sparky;
                    else if ( t == "whispywoods" ) mt = game::MonsterType::WhispyWoods;
                    else if ( t == "apple" )       mt = game::MonsterType::Apple;
                    else continue;

                    game::SpawnSpec spec{};
                    spec.type = mt; spec.x = r.x; spec.y = r.y;
                    spec.dir = ( r.dir < 0 ? -1 : ( r.dir > 0 ? +1 : 0 ) );
                    spec.attack = r.attack; spec.move = r.move;

                    auto mon = game::MonsterFactory::Create ( mt , m_World.WorldRectPx ( ) , &m_World.Collision ( ) , spec );
                    if ( !mon ) continue;

                    // 텍스처/비주얼 사이즈/소스 (기존 GameApp 구현 그대로)
                    if ( m_EnemiesTex.srv ) mon->SetSpriteSheet ( &m_EnemiesTex );
                    mon->SetVisualSize ( 32.f , 32.f );
                    RECT src{};
                    switch ( mt ) {
                    case game::MonsterType::WaddleDee: src = RECT{ 8, 8, 40, 40 };     break;
                    case game::MonsterType::WaddleDoo: src = RECT{ 8, 40, 40, 72 };    break;
                    case game::MonsterType::HotHead:   src = RECT{ 8, 136, 40, 168 };  break;
                    case game::MonsterType::Sparky:    src = RECT{ 8, 168, 40, 200 };  break;
                    default: break;
                    }
                    mon->SetSpriteSrc ( src );

                    // --- WhispyWoods: 큰 고정형 보스 콜라이더 ---
                    if ( mt == game::MonsterType::WhispyWoods ) {
                        mon->SetSize ( 6.f * game::TILE_PX , 8.f * game::TILE_PX ); // 96x128px
                        // 필요 시 비주얼 스케일도 함께 키우려면 아래 주석 해제
                        // mon->SetVisualSize( 6.f * game::TILE_PX, 8.f * game::TILE_PX );
                    }

                    // 몬스터 → 투사체/히트볼륨 스포너 콜백
                    mon->SetProjectileSpawnerId ( [ this ] ( const std::string& arche , const engine::Vec2& pos ,
                        const engine::Vec2& vel , game::ProjOwner owner ) {
                            game::ProjectileSystem::SpawnDesc sd{};
                            sd.archetype = arche; sd.owner = owner; sd.pos = pos; sd.dirOrVel = vel; sd.treatAsDirection = false;
                            m_projSys.Spawn ( sd );
                    } );
                    mon->SetTargetQuery ( [ this ] ( ) { return m_Player ? m_Player->Center ( ) : engine::Vec2{}; } );
                    mon->SetHitVolumeSpawner ( [ this ] ( const std::string& arche , int ownerId , int facing , const engine::Vec2& anchor ) {
                        game::HitVolumeSystem::SpawnDesc sd{ arche, ownerId, facing, anchor };
                        m_hitSys.Spawn ( sd );
                    } );
                    mon->SetMonsterSpawner ( [ this ] ( MonsterType t , const engine::Vec2& pos , const SpawnSpec& spec ) {
                        auto s = spec; s.type = t; s.x = pos.x; s.y = pos.y; m_pendingMonsterSpawns.push_back ( s );
                    } );

                    m_Monsters.push_back ( std::move ( mon ) );
                }
            }
        }

        // ---- Doors: StageCSV 사용 ----
        m_Doors.clear ( );
        if ( !desc.doors.empty ( ) ) {
            std::vector<game::DoorCSV> tmp;
            if ( game::LoadDoorsCSV ( desc.doors.c_str ( ) , tmp ) ) m_Doors = std::move ( tmp );
        }

        // 전투 시스템은 월드/콜라이더/플레이어 배치 이후에 재구성
        initCombatSystems ( );
        return true;
    }

    void PlaySession::initPlayerAndCamera ( const RECT& rcClient ) {
        if ( !m_Scene ) return;
        m_Player = m_Scene->Spawn<game::Player> ( rcClient );
        m_PlayerFSM.Init ( &m_Player->Body ( ) , &m_World.Collision ( ) , m_Player->Animator ( ) , m_playerFsmCfg );

        const int w = rcClient.right - rcClient.left;
        const int h = rcClient.bottom - rcClient.top;
        m_Cam.SetScreenSize ( w , h );
        m_Cam.SetSmoothSpeed ( 10.f );
        m_Cam.SetPixelSnap ( true );
        m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.SnapImmediate ( );
    }

    void PlaySession::updateCameraBoundsForWorld ( int sw , int sh ) {
        RECT wr0 = m_World.WorldRectPx ( );
        if ( wr0.right <= wr0.left || wr0.bottom <= wr0.top ) return;

        const int viewW_world = sw;
        const int viewH_world = sh;
        const int padWorld = game::TILE_PX / 2;
        RECT wr = wr0;
        const int wldW = wr.right - wr.left , wldH = wr.bottom - wr.top;
        if ( wldW > viewW_world ) { wr.left += padWorld; wr.right -= padWorld; }
        if ( wldH > viewH_world ) { wr.top += padWorld; wr.bottom -= padWorld; }
        // 보스 락 중이면 아레나로, 아니면 월드(패드 적용)
        if ( m_bossCamLocked && m_hasBossArena ) {
            m_Cam.SetWorldRect ( m_bossArena );
        }
        else {
            m_Cam.SetWorldRect ( wr );
        }
    }
} // namespace game
