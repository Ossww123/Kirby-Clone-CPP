#include "game/PlaySession.h"

#include <algorithm>
#include <cmath>
#include <cwchar>

#include "engine/D3D11Renderer.h"
#include "engine/TextureLoader.h"
#include "engine/StringConv.h"
#include "engine/Collision.h"
#include "engine/StringConv.h"
#include "game/AnimCSV.h"
#include "game/GameConfig.h"

namespace game {

    PlaySession::~PlaySession ( ) = default;

    void PlaySession::Initialize ( const CreateDesc& d ) {
        m_Renderer = d.renderer;
        m_Batch = d.batch;
        m_Debug = d.debug;
        m_TextHUD = d.textHUD;
        m_Scene = d.scene;

        // 플레이어/카메라 초기화 (기존 GameApp::InitPlayerAndCamera 이식)
        initPlayerAndCamera ( d.rcClient );

        // 텍스처 로드 (기존 GameApp::Init 일부 이식)
        auto* d3d = dynamic_cast< engine::D3D11Renderer* >( m_Renderer );
        if ( d3d ) {
            if ( !m_PlayerTex.srv ) {
                engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/player.png" , &m_PlayerTex );
            }
            if ( !m_EnemiesTex.srv ) {
                engine::LoadTextureWIC ( d3d->Device ( ) , L"assets/enemies.png" , &m_EnemiesTex );
            }
            if ( !m_WhiteTex.srv ) {
                engine::CreateSolidTexture1x1 ( d3d->Device ( ) , 0xFFFFFFFFu , &m_WhiteTex );
            }
        }

        if ( m_Player && m_PlayerTex.srv ) {
            m_Player->SetTexture ( m_PlayerTex );
            m_Player->SetSize ( float ( game::PLAYER_COLL_PX ) , float ( game::PLAYER_COLL_PX ) );
            m_Player->SetVisualSize ( 32.f , 32.f );
            // 애니 CSV (실패 시 스킵)
            game::LoadAnimCSV ( "assets/player_anim.csv" , m_Player->Animator ( ) , /*clear=*/true );
            if ( auto* a = m_Player->Animator ( ) ) a->Play ( "Idle" , true );
        }

        registerDefaultFactories ( );
    }

    void PlaySession::OnResize ( int sw , int sh ) {
        if ( sw <= 0 || sh <= 0 ) return;

        // 카메라 화면 크기 갱신 + 스냅 (기존 GameApp::OnResize 일부 이식)
        m_Cam.SetScreenSize ( sw , sh );
        updateCameraBoundsForWorld ( sw , sh );
        m_Cam.SnapImmediate ( );

        if ( m_Batch ) m_Batch->OnResize ( sw , sh );
        if ( m_Debug ) m_Debug->OnResize ( sw , sh );
    }

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

    void PlaySession::FixedUpdate ( double fixedDt , const engine::Input& input ) {
        // 1) 플레이어 FSM
        m_PlayerFSM.Step ( fixedDt , input );
        std::vector<game::PlayerEvent> evs; m_PlayerFSM.DrainEvents ( evs );
        if ( !evs.empty ( ) ) handlePlayerEvents ( evs );
        // 2) 몬스터
        updateMonsters ( fixedDt , input );
        // 3) 전투(타깃 구축 → 시스템 스텝 → 히트 적용)
        std::vector<game::ProjectileSystem::Target> projT;
        std::vector<game::HitVolumeSystem::Target> hvT;
        buildTargets ( projT , hvT );
        m_projSys.Step ( fixedDt , projT );
        m_hitSys.Step ( fixedDt , hvT );
        std::vector<game::ProjectileSystem::HitEvent> phits; m_projSys.DrainHitEvents ( phits );
        std::vector<game::HitVolumeSystem::HitEvent> hvHits; m_hitSys.DrainHitEvents ( hvHits );
        if ( !phits.empty ( ) ) applyProjectileHits ( phits );
        if ( !hvHits.empty ( ) ) applyHitVolumeHits ( hvHits );
        // 4) 애니/카메라/스폰
        if ( m_Player && m_Player->Animator ( ) ) m_Player->Animator ( )->Update ( static_cast< float >( fixedDt ) );
        if ( m_Player ) m_Cam.SetLookAt ( m_Player->Center ( ) );
        m_Cam.Update ( fixedDt );
        flushPendingSpawns ( );
        updateTransition ( fixedDt );
        // ---- Fade 진행 ----
        if ( m_fade.mode != Fade::None && m_fade.dur > 0.f ) {
            m_fade.t += float ( fixedDt );
            if ( m_fade.t >= m_fade.dur ) {
                // 끝
                m_fade.t = m_fade.dur;
                m_fade.mode = Fade::None;
            }
        }
    }

    void PlaySession::RenderParallaxBG ( int ox , int oy , int sw , int sh ) {
        if ( !m_BgTex.srv ) return;

        // 기존 GameApp::RenderParallaxBG 이식 (월드 경계/패드/비율 동일)
        const RECT wr = m_World.WorldRectPx ( );
        const int worldW = wr.right - wr.left;
        const int worldH = wr.bottom - wr.top;
        const int pad = game::TILE_PX / 2;

        auto calcParallaxOffset = [ & ] ( int camPos , int screenSize , int bgSize , int worldSize , int worldMin ) {
            const int camMax = std::max ( 0 , worldSize - screenSize - 2 * pad );
            const int bgMax = std::max ( 0 , bgSize - screenSize );
            const int bgPad = std::min ( pad , bgMax / 2 );
            const int bgMaxEff = std::max ( 0 , bgMax - 2 * bgPad );
            const int camEff = std::clamp ( camPos - ( worldMin + pad ) , 0 , camMax );
            const int bgOffset = ( camMax > 0 ) ? ( int ) std::lround ( ( double ) camEff * bgMaxEff / camMax ) : 0;
            return ( bgOffset + bgPad ) / game::SCALE;
            };

        const int srcLeft = calcParallaxOffset ( ox , sw , m_BgTex.width * game::SCALE , worldW , wr.left );
        const int srcTop = calcParallaxOffset ( oy , sh , m_BgTex.height * game::SCALE , worldH , wr.top );
        const int viewW_tex = sw / game::SCALE;
        const int viewH_tex = sh / game::SCALE;

        RECT src{
            std::clamp ( srcLeft, 0, std::max ( 0, m_BgTex.width - viewW_tex ) ),
            std::clamp ( srcTop , 0, std::max ( 0, m_BgTex.height - viewH_tex ) ),
            0, 0
        };
        src.right = src.left + viewW_tex;
        src.bottom = src.top + viewH_tex;

        m_Batch->Draw ( m_BgTex , 0.f , 0.f , ( float ) sw , ( float ) sh , &src , 0xFFFFFFFF );
    }

    void PlaySession::RenderWorld ( int ox , int oy , int sw , int sh ) {
        // 1) 타일
        m_World.RenderVisible ( *m_Batch , ox , oy , sw , sh );

        // 2) 플레이어
        if ( m_Player ) {
            const auto& tex = m_Player->Texture ( );
            if ( tex.srv ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                float vw , vh;       m_Player->GetVisualSize ( vw , vh );
                const float sx = ( ( px + pw * 0.5f ) - vw * 0.5f - ox );
                const float sy = ( ( py + ph ) - vh - oy );
                RECT src = m_Player->Animator ( )->CurrentSrc ( ); // 기존 GameApp 코드와 동일
                m_Batch->Draw ( tex , sx , sy , vw * game::SCALE , vh * game::SCALE ,
                              ( src.right > src.left ) ? &src : nullptr , 0xFFFFFFFF );
            }
        }

        // 3) 몬스터
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            const auto* tex = m->TexturePtr ( );
            if ( !tex || !tex->srv ) continue;
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            float vw , vh;       m->GetVisualSize ( vw , vh );
            const float sx = ( ( mx + mw * 0.5f ) - vw * 0.5f - ox );
            const float sy = ( ( my + mh ) - vh - oy );
            RECT src = m->SpriteSrc ( );
            m_Batch->Draw ( *tex , sx , sy , vw * game::SCALE , vh * game::SCALE , &src , 0xFFFFFFFF );
        }
    }

    void PlaySession::RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh , bool drawEnabled ) {
        if ( !drawEnabled || !m_Debug ) return;

        const int GRID = game::GRID_PX;
        const int wx0 = ox , wy0 = oy , wx1 = ox + sw , wy1 = oy + sh;
        int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
        for ( int x = gx; x <= wx1; x += GRID ) m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , RGB ( 60 , 60 , 60 ) );
        for ( int y = gy; y <= wy1; y += GRID ) m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , RGB ( 60 , 60 , 60 ) );

        // 콜라이더 와이어(기존 코드) :contentReference[oaicite:3]{index=3}
        m_World.Collision ( ).DebugDraw ( *m_Debug , ox , oy , RGB ( 255 , 60 , 60 ) , RGB ( 255 , 200 , 0 ) );

        // 플레이어 AABB
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            m_Debug->WorldRect ( px , py , pw , ph , ox , oy , RGB ( 0 , 255 , 0 ) );
        }

        // 몬스터 디버그 (헬스바/바운딩 박스 등)
        for ( const auto& m : m_Monsters ) {
            if ( m && m->Alive ( ) ) m->RenderDebug ( m_Debug , ox , oy );
        }
        
        // 투사체 시스템 디버그
        // (ProjectileSystem에 DebugDraw가 이미 있다면 이 한 줄이면 끝)
        m_projSys.DebugDraw ( *m_Debug , ox , oy );
        
        // 히트볼륨 시스템 디버그
        m_hitSys.DebugDraw ( *m_Debug , ox , oy );

        for ( const auto& d : m_Doors ) {
            m_Debug->WorldRect ( d.x , d.y , d.w , d.h , ox , oy , RGB ( 0 , 200 , 255 ) );
        }
        m_Debug->Flush ( );
    }


    void PlaySession::RenderHUD ( int fps , double fixedDt )
    {
        if ( !m_TextHUD ) return;
        m_TextHUD->Begin ( );

        // 1) FPS / dt
        wchar_t buf[ 128 ];
        std::swprintf ( buf , _countof ( buf ) , L"FPS:%d  dt:%.3f" , fps , fixedDt );
        m_TextHUD->DrawTextLine ( buf , 8.f , 8.f );

        // 2) 상태 이름들 (Move / Action / Overlay)
        wchar_t st[ 64 ];
        std::swprintf ( st , _countof ( st ) ,
                      L"STATE  M:%S  A:%S  Z:%S" ,
                      m_PlayerFSM.MoveStateName ( ) ,
                      m_PlayerFSM.ActionStateName ( ) ,
                      m_PlayerFSM.OverlayStateName ( ) );
        m_TextHUD->DrawTextLine ( st , 8.f , 28.f );

        // 3) FSM debug snapshot
        auto dbg = m_PlayerFSM.GetDebug ( );

        wchar_t line[ 256 ];
        std::swprintf ( line , _countof ( line ) ,
                      L"VEL: (%+07.1f, %+07.1f)  grounded(raw:%d / stable:%d)  onewayIgnore:%d" ,
                      dbg.vx , dbg.vy ,
                      dbg.groundedRaw ? 1 : 0 ,
                      dbg.groundedStable ? 1 : 0 ,
                      dbg.ignoreOneWay ? 1 : 0 );
        m_TextHUD->DrawTextLine ( line , 8.f , 48.f );

        std::swprintf ( line , _countof ( line ) ,
                      L"Timers  coyote:%.3f  buffer:%.3f  drop:%.3f  groundHold:%.3f" ,
                      dbg.coyoteT , dbg.bufferT , dbg.dropT , dbg.groundHoldT );
        m_TextHUD->DrawTextLine ( line , 8.f , 68.f );

        std::swprintf ( line , _countof ( line ) ,
                      L"AABB L:%d T:%d R:%d B:%d   prevBottom:%d" ,
                      dbg.lastAABB.left , dbg.lastAABB.top ,
                      dbg.lastAABB.right , dbg.lastAABB.bottom , dbg.prevBottom );
        m_TextHUD->DrawTextLine ( line , 8.f , 88.f );

        // 4) 발 밑 좌표/타일 인덱스
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            const int footX = px + pw / 2;
            const int footY = py + ph;
            const int tileSize = m_World.TileW ( );
            const int tx = footX / tileSize;
            const int ty = footY / tileSize;
            std::swprintf ( line , _countof ( line ) ,
                          L"Foot: (%d, %d)  Tile: (%d, %d)" , footX , footY , tx , ty );
            m_TextHUD->DrawTextLine ( line , 8.f , 108.f );
        }

        // 5) 카메라/몬스터/HP
        auto [ox , oy] = m_Cam.OffsetInt ( );
        wchar_t cam[ 128 ];
        std::swprintf ( cam , _countof ( cam ) ,
                      L"Cam  off:(%d,%d)  look:(%.1f, %.1f)" ,
                      ox , oy , m_Cam.GetLookAt ( ).x , m_Cam.GetLookAt ( ).y );
        m_TextHUD->DrawTextLine ( cam , 8.f , 128.f );

        wchar_t mons[ 64 ];
        std::swprintf ( mons , _countof ( mons ) , L"Monsters: %zu" , m_Monsters.size ( ) );
        m_TextHUD->DrawTextLine ( mons , 8.f , 148.f );

        wchar_t hpLine[ 64 ];
        std::swprintf ( hpLine , _countof ( hpLine ) , L"HP: %d" , dbg.hp );
        m_TextHUD->DrawTextLine ( hpLine , 8.f , 168.f );

        // 6) 능력 이름
        const wchar_t* abilityName = L"None";
        switch ( dbg.ability ) {
        case game::Ability::Fire:  abilityName = L"Fire";  break;
        case game::Ability::Spark: abilityName = L"Spark"; break;
        case game::Ability::Beam:  abilityName = L"Beam";  break;
        default: break;
        }

        // Kirby 플래그 (facing / mouthFull / ability)
        std::swprintf ( line , _countof ( line ) ,
                      L"Kirby  facing:%d  mouthFull:%d  ability:%ls" ,
                      dbg.facing , dbg.mouthFull ? 1 : 0 , abilityName );
        m_TextHUD->DrawTextLine ( line , 8.f , 188.f );

        // 액션 타이머
        std::swprintf ( line , _countof ( line ) ,
                      L"Action  inhaleT:%.2f  spitLockT:%.2f" ,
                      dbg.inhaleT , dbg.spitLockT );
        m_TextHUD->DrawTextLine ( line , 8.f , 208.f );

        // (선택) 현재 스테이지 파일명 표시
        {
            wchar_t stage[ 256 ];
            // m_stageJsonPath는 UTF-8일 가능성 → 간단히 멀티바이트로 캐스팅(한글 경로면 ToWide 사용 권장)
            std::swprintf ( stage , _countof ( stage ) , L"Stage: %hs" , m_stageJsonPath.c_str ( ) );
            m_TextHUD->DrawTextLine ( stage , 8.f , 228.f );
        }

        m_TextHUD->End ( );
    }


    static uint32_t MakeARGB ( uint8_t a , uint32_t rgb ) {
        // SpriteBatch가 0xAARRGGBB로 받아 그려주는 전제
        return ( uint32_t ( a ) << 24 ) | ( rgb & 0x00FFFFFFu );
    }

    void PlaySession::RenderOverlayFade ( int sw , int sh ) {
        if ( !m_Batch || !m_WhiteTex.srv ) return;
        if ( m_fade.mode == Fade::None || m_fade.dur <= 0.f ) return;

        float t = std::clamp ( m_fade.t / std::max ( 0.0001f , m_fade.dur ) , 0.f , 1.f );
        float alpha = ( m_fade.mode == Fade::Out ) ? t : ( 1.f - t ); // Out: 0→1, In: 1→0
        uint8_t a = ( uint8_t ) std::lround ( alpha * 255.f );

        m_Batch->Draw ( m_WhiteTex , 0.f , 0.f , ( float ) sw , ( float ) sh , nullptr , MakeARGB ( a , m_fade.rgb ) );
    }

    // ==== 이벤트 큐 외부 전달(임시) ====
    void PlaySession::DrainPlayerEvents ( std::vector<game::PlayerEvent>& out ) {
        if ( m_pendingPlayerEvents.empty ( ) ) { out.clear ( ); return; }
        out.swap ( m_pendingPlayerEvents );
    }

    // ----- Fade API -----
    void PlaySession::StartFadeIn ( float seconds , uint32_t rgb ) {
        m_fade.mode = Fade::In; m_fade.t = 0.f; m_fade.dur = std::max ( 0.f , seconds ); m_fade.rgb = rgb;
    }
    void PlaySession::StartFadeOut ( float seconds , uint32_t rgb ) {
        m_fade.mode = Fade::Out; m_fade.t = 0.f; m_fade.dur = std::max ( 0.f , seconds ); m_fade.rgb = rgb;
    }

    void PlaySession::StartTransitionTo ( const std::string& target , float o , float i )
    {
        if ( target.empty ( ) ) return;
        m_trans = {};
        m_trans.state = Transition::FadingOut;
        m_trans.target = target;
        m_trans.fadeOut = o; m_trans.fadeIn = i;
        StartFadeOut ( o );
    }

    // ----- 내부 유틸 -----

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
        m_Cam.SetWorldRect ( wr );
    }

    // ==== 전투 시스템 초기화 & 팩토리 등록 ====
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

    // ===== 몬스터/전투 유틸 =====
    void PlaySession::updateMonsters ( double fixedDt , const engine::Input& input ) {
        for ( auto& m : m_Monsters ) if ( m ) m->Update ( fixedDt , input );
    }

    // ===== 플레이어 이벤트 처리 =====
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
            if ( ev.isCapture ) { // 빨아들이기 캡쳐
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
            m_Monsters.push_back ( std::move ( mon ) );
        }
    }
    bool PlaySession::checkDoorInteract ( )
    {
        if ( m_trans.state != Transition::Idle ) return false; // 전환 중엔 무시
        if ( !m_Player || m_Doors.empty ( ) ) return false;

        int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
        RECT paabb{ px,py,px + pw,py + ph };

        for ( const auto& d : m_Doors ) {
            RECT daabb{ d.x, d.y, d.x + d.w, d.y + d.h };
            if ( engine::physics::Overlap ( paabb , daabb ) ) {
                StartTransitionTo ( d.target );
                return true;
            }
        }
        return false;
    }

    void PlaySession::updateTransition ( double )
    {
        switch ( m_trans.state ) {
        case Transition::Idle: break;
        case Transition::FadingOut:
            if ( !IsFading ( ) ) {
                m_trans.state = Transition::Loading;
                // 실제 로드
                LoadStage ( m_trans.target.c_str ( ) );
                StartFadeIn ( m_trans.fadeIn );
                m_trans.state = Transition::FadingIn;
            }
            break;
        case Transition::FadingIn:
            if ( !IsFading ( ) ) m_trans.state = Transition::Idle;
            break;
        case Transition::Loading: default: break;
        }
    }

} // namespace game
