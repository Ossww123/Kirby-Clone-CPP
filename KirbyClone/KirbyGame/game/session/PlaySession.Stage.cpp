// PlaySession.Stage.cpp
//
// Responsibility: Stage loading and world setup (tiles/monsters/background/player/camera bounds).
// Non-Goals    : Renderer creation, gameplay rules beyond initial spawn.
// Call-Context : Called by PlaySession to (re)load a stage and rebuild systems.
//

#include <algorithm>
#include <cctype>

#include "game/session/PlaySession.h"

#include "engine/render/D3D11Renderer.h"
#include "engine/render/TextureLoader.h"
#include "engine/util/StringConv.h"
#include "engine/util/Types.h"
#include "engine/world/TileSet.h"

#include "game/data/StageCSV.h"
#include "game/data/StageDesc.h"
#include "game/entities/monsters/MonsterFactory.h"
#include "game/entities/player/Player.h"
#include "game/data/GameConfig.h"
#include "game/session/SpawnSelector.h"
#include "game/session/HubCoverUnlock.h"

#ifndef DBGLOG
#include <string>
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) { ::OutputDebugStringW ( msg ); ::OutputDebugStringW ( L"\n" ); }
static void ShowAssetLoadWarning ( const char* kind , const char* path )
{
    // kind: "tilemap", "tiledefs", "monsters" 같은 용도 이름
    // path: 실패한 파일 경로 (UTF-8 / narrow)

    std::wstring wKind = engine::ToWide ( kind ? kind : "" );
    std::wstring wPath = engine::ToWide ( path ? path : "(null)" );

    std::wstring msg = L"[Stage] Failed to load ";
    msg += wKind;
    msg += L" file:\n";
    msg += wPath;

    DBGLOG ( msg.c_str ( ) ); // 디버그 출력도 같이

    ::MessageBoxW (
        nullptr ,
        msg.c_str ( ) ,
        L"Asset Load Error" ,
        MB_OK | MB_ICONWARNING
    );
}
#endif

namespace game {

    bool PlaySession::LoadStage ( const char* jsonPath ) {
        // ---- stage.json ----
        m_stageJsonPath = jsonPath ? jsonPath : m_stageJsonPath;

        game::StageDesc desc{};
        if ( !game::LoadStageDesc ( m_stageJsonPath.c_str ( ) , desc ) ) {
            std::wstring msg = L"[Stage] LoadStage: LoadStageDesc FAILED for '" +
                engine::ToWide ( m_stageJsonPath ) + L"'";
            DBGLOG ( msg.c_str ( ) );
            return false;
        }

        {
            std::wstring msg = L"[Stage] StageDesc.id='" + engine::ToWide ( desc.id ) + L"'";
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] tileset     = " + engine::ToWide ( desc.tileset );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] tiledefs    = " + engine::ToWide ( desc.tiledefs );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] tilemap     = " + engine::ToWide ( desc.tilemap );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] monsters    = " + engine::ToWide ( desc.monsters );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] items       = " + engine::ToWide ( desc.items );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] player_start= " + engine::ToWide ( desc.player_start );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] background  = " + engine::ToWide ( desc.background );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] doors       = " + engine::ToWide ( desc.doors );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] cover_tile  = " + engine::ToWide ( desc.cover_tilemap );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] unlocks     = " + engine::ToWide ( desc.unlocks );
            DBGLOG ( msg.c_str ( ) );
        }
        {
            std::wstring msg = L"[Stage] layers      = " + engine::ToWide ( desc.layers );
            DBGLOG ( msg.c_str ( ) );
        }

        m_stageId = desc.id;

        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( !d3d ) return false;

        // ---- Tile/Map define ----
        m_World.LoadTileset ( d3d->Device ( ) , engine::ToWide ( desc.tileset ).c_str ( ) , 16 , 16 );
        m_World.SetWorldTileSize ( game::TILE_PX , game::TILE_PX );

        int mw = 0 , mh = 0; std::vector<int> ids;
        if ( !game::LoadTileMapCSV ( desc.tilemap.c_str ( ) , mw , mh , ids ) ) {
            ShowAssetLoadWarning ( "tilemap" , desc.tilemap.c_str ( ) );
            return false;
        }
        m_World.SetMapFromMemory ( mw , mh , ids.data ( ) );

        std::vector<game::TileDefCSV> tdefs;
        if ( !game::LoadTileDefsCSV ( desc.tiledefs.c_str ( ) , tdefs ) || tdefs.empty ( ) ) {
            ShowAssetLoadWarning ( "tiledefs" , desc.tiledefs.c_str ( ) );
            return false;
        }

        const int cw = 16 , ch = 16;
        for ( const auto& r : tdefs ) {
            engine::TileDef d{};
            d.solid = ( r.solid != 0 );
            d.oneway = ( r.oneway != 0 );
            if ( r.gx >= 0 && r.gy >= 0 )
                d.src = engine::IntRect{ r.gx * cw, r.gy * ch, r.gx * cw + cw, r.gy * ch + ch };
            m_World.DefineTile ( r.id , d );
        }


        // ---- Cover layer (hub blockers) ----
        bool rebuilt = false;
        if ( !desc.cover_tilemap.empty ( ) ) {
            int cw = 0 , ch = 0; std::vector<int> cids;
            if ( game::LoadTileMapCSV ( desc.cover_tilemap.c_str ( ) , cw , ch , cids ) ) {
                m_World.SetCoverFromMemory ( cw , ch , cids.data ( ) );
            }
            else {
                ShowAssetLoadWarning ( "cover_tilemap" , desc.cover_tilemap.c_str ( ) );
                m_World.ClearCover ( );
            }
        }
        else {
            m_World.ClearCover ( ); // 허브가 아닌 스테이지에서 커버 잔류 방지
        }

        // ---- Hub unlocks (erase cover tiles for cleared stages) ----
        if ( m_Session && desc.id == "t1/hub" ) {
            game::ApplyHubCoverUnlocks ( desc , &m_Session->Data ( ) , m_World ); // 내부에서 RebuildColliders() 호출
            rebuilt = true;
        }

        if ( !rebuilt ) {
            m_World.RebuildColliders ( );
        }


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
            if ( engine::LoadTextureWIC ( d3d->Device ( ) , engine::ToWide ( desc.background ).c_str ( ) , &bg ) ) {
                m_BgTex = bg;
            }
            else {
                m_BgTex = {};
            }
        }
        else {
            m_BgTex = {};
        }

        // ---- Optional tile layers (from stage.json "layers") ----
        m_TileLayers.clear ( );

        if ( !desc.layers.empty ( ) ) {
            std::vector<game::TileLayerCSV> ldefs;
            if ( game::LoadTileLayersCSV ( desc.layers.c_str ( ) , ldefs ) ) {
                m_TileLayers.reserve ( ldefs.size ( ) );

                for ( const auto& r : ldefs ) {
                    game::TileLayerRuntime layer{};
                    layer.name = r.name;
                    layer.offsetX = r.offsetPxX;
                    layer.offsetY = r.offsetPxY;
                    layer.collides = ( r.collides != 0 );
                    layer.z = r.z;

                    // Tileset 로드
                    if ( !layer.tiles.LoadAtlas ( d3d->Device ( ) ,
                        engine::ToWide ( r.tileset ).c_str ( ) ,
                        16 , 16 ) ) {
                        continue; // 이 레이어는 스킵
                    }
                    layer.tiles.SetWorldTileSize ( game::TILE_PX , game::TILE_PX );

                    // TileDefs 로드 → TileSet.Define
                    std::vector<game::TileDefCSV> defs;
                    if ( game::LoadTileDefsCSV ( r.tiledefs.c_str ( ) , defs ) && !defs.empty ( ) ) {
                        const int cw = 16 , ch = 16;
                        for ( const auto& td : defs ) {
                            engine::TileDef d{};
                            d.solid = ( td.solid != 0 );
                            d.oneway = ( td.oneway != 0 );
                            if ( td.gx >= 0 && td.gy >= 0 ) {
                                d.src = engine::IntRect{
                                    td.gx * cw ,
                                    td.gy * ch ,
                                    td.gx * cw + cw ,
                                    td.gy * ch + ch
                                };
                            }
                            layer.tiles.Define ( td.id , d );
                        }
                    }

                    // TileMap 로드 → TileMap.LoadFromMemory
                    int lw = 0 , lh = 0;
                    std::vector<int> ids;
                    if ( !game::LoadTileMapCSV ( r.tilemap.c_str ( ) , lw , lh , ids ) ) {
                        continue;
                    }
                    layer.map.LoadFromMemory ( lw , lh , ids.data ( ) );

                    m_TileLayers.push_back ( std::move ( layer ) );
                }

                // z 오름차순 정렬 (작을수록 BG, 클수록 FG)
                std::sort ( m_TileLayers.begin ( ) , m_TileLayers.end ( ) ,
                            [ ] ( const game::TileLayerRuntime& a , const game::TileLayerRuntime& b ) {
                                return a.z < b.z;
                            } );
            }
        }


        // ---- Spawn resolve (override → spawns.csv → save.lastSpawn → default → player_start.csv) ----
        if ( m_Player ) {
            game::ResolvedSpawn rs{};
            const char* ov = m_trans.spawn.empty ( ) ? nullptr : m_trans.spawn.c_str ( );
            if ( game::ResolveSpawn ( desc , ov , m_Session ? &m_Session->Data ( ) : nullptr , rs ) ) {
                m_Player->SetPosition ( rs.x , rs.y );
                m_Player->Body ( ).SetVelocity ( { 0.f, 0.f } );
                m_PlayerFSM.SetFacing ( rs.dir );

                updateCameraBoundsForWorld (
                    m_Renderer ? m_Renderer->GetBackbufferSize ( ).w : 0 ,
                    m_Renderer ? m_Renderer->GetBackbufferSize ( ).h : 0
                );
                m_Cam.SetLookAt ( { rs.x, rs.y } );
                m_Cam.SnapImmediate ( );
            }
        }

        // ---- Items (clear emblem, pickups) ----
        m_Items.clear ( );
        if ( !desc.items.empty ( ) ) {
            std::vector<game::ItemCSV> itemDefs;
            if ( game::LoadItemsCSV ( desc.items.c_str ( ) , itemDefs ) ) {
                const int itemSize = game::TILE_PX; // 한 타일 크기(16x16) 픽업으로 가정
                const int half = itemSize / 2;

                for ( const auto& ic : itemDefs ) {
                    // type 문자열 → 내부 Kind 매핑
                    std::string t = ic.type;
                    std::transform ( t.begin ( ) , t.end ( ) , t.begin ( ) ,
                                     [ ] ( unsigned char c ) { return static_cast< char >( std::tolower ( c ) ); } );

                    ItemRuntime it{};

                    if ( t == "clear_emblem" || t == "clear" || t == "emblem" ) {
                        it.kind = ItemRuntime::Kind::ClearEmblem;
                    }
                    else {
                        continue; // 아직은 클리어 엠블렘만 처리
                    }

                    const int cx = static_cast< int >( ic.x );
                    const int cy = static_cast< int >( ic.y );

                    // (cx,cy)를 중심으로 하는 itemSize x itemSize 박스
                    it.x = cx - half;
                    it.y = cy - half;
                    it.w = itemSize;
                    it.h = itemSize;
                    it.collected = false;

                    m_Items.push_back ( it );
                }
            }
            else {
                ShowAssetLoadWarning ( "items" , desc.items.c_str ( ) );
            }
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
                    if ( m_EnemiesTex.srv ) mon->SetTexture ( &m_EnemiesTex );
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
                        [[maybe_unused]] const int hvId = m_hitSys.Spawn ( sd );
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

        // 임시 페이드 인
        if ( desc.id == "t1/hub" && m_trans.state == Transition::Idle ) {
            StartFadeIn ( 0.6f , 0xFFFFFFu , game::Z::OverlayTop );
        }

        return true;
    }

    void PlaySession::initPlayerAndCamera ( const engine::IntRect& rcClient ) {
        m_Player = std::make_unique<game::Player> ( rcClient /* + 필요하면 나머지 인자 */ );
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
