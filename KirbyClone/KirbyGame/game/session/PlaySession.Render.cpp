// PlaySession.Render.cpp
//
// Responsibility: Session render paths (parallax background, world, debug, HUD, fade).
// Non-Goals    : Resource loading policy or scene ownership.
// Call-Context : Called by GameApp between Begin/EndFrame.

#include "game/session/PlaySession.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/render/D3D11DebugDraw.h"
#include "engine/core/RenderSystem.h"
#include "engine/render/D3D11DebugDrawAdapter.h"
#include "engine/render/DWriteText.h"
#include "engine/util/Types.h"
#include "game/data/GameConfig.h"
#include "game/entities/player/Player.h"
#include "game/entities/monsters/Monster.h"
#include "engine/platform/win32/ColorUtil.h"
#include "engine/physics/CollisionDebugDraw.h"
#include "game/debugdraw/MonsterDebugDraw.h"

#include <algorithm>
#include <cwchar>

namespace {
    // SpriteBatch expects ARGB (0xAARRGGBB).
    inline uint32_t MakeARGB ( uint8_t a , uint32_t rgb ) {
        return ( uint32_t ( a ) << 24 ) | ( rgb & 0x00FFFFFFu );
    }

    inline uint32_t ARGB ( uint8_t a , uint8_t r , uint8_t g , uint8_t b ) {
        return ( uint32_t ( a ) << 24 ) | ( uint32_t ( r ) << 16 ) | ( uint32_t ( g ) << 8 ) | uint32_t ( b );
    }
}

namespace game {

    void PlaySession::RenderParallaxBG ( int ox , int oy , int sw , int sh ) {
        if ( !m_BgTex.srv ) return;

        // World rect / padding
        const engine::IntRect wr = m_World.WorldRectPx ( );
        const int worldW = wr.r - wr.l;
        const int worldH = wr.b - wr.t;
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

        const int srcLeft = calcParallaxOffset ( ox , sw , m_BgTex.width * game::SCALE , worldW , wr.l );
        const int srcTop = calcParallaxOffset ( oy , sh , m_BgTex.height * game::SCALE , worldH , wr.t );
        const int viewW_tex = sw / game::SCALE;
        const int viewH_tex = sh / game::SCALE;

        engine::IntRect src{
            std::clamp ( srcLeft , 0 , std::max ( 0 , m_BgTex.width - viewW_tex ) ),
            std::clamp ( srcTop  , 0 , std::max ( 0 , m_BgTex.height - viewH_tex ) ),
            0, 0
        };
        src.r = src.l + viewW_tex;
        src.b = src.t + viewH_tex;

        m_RenderSys->DrawSprite (
            m_BgTex , 0.f , 0.f , ( float ) sw , ( float ) sh , &src ,
            engine::win32::RGBA8 ( 255 , 255 , 255 ) ,
            0.f , 0.f , 0.f ,
            /*z*/ game::Z::BG , engine::BlendMode::Alpha , engine::SamplerMode::Linear
         );
    }

    void PlaySession::RenderWorld ( int ox , int oy , int sw , int sh ) {
        if ( !m_RenderSys ) return;

        auto& batch = m_RenderSys->Batch ( );

        // 0) Tile layers with z < 0 (background relative to base world)
        for ( const auto& layer : m_TileLayers ) {
            if ( layer.z < 0 ) {
                layer.RenderScaled ( batch , ox , oy , sw , sh , game::SCALE );
            }
        }

        // 1) Base world tiles (WorldSystem: ground + cover)
        m_World.RenderVisible ( batch , ox , oy , sw , sh );

        // 1.5) Tile layers with z >= 0 (overlays)
        for ( const auto& layer : m_TileLayers ) {
            if ( layer.z >= 0 ) {
                layer.RenderScaled ( batch , ox , oy , sw , sh , game::SCALE );
            }
        }

        // 2) Player
        if ( m_Player ) {
            const engine::Tex2D* tex = m_Player->TexturePtr ( );
            if ( tex && tex->srv ) {
                int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
                float vw , vh;         m_Player->GetVisualSize ( vw , vh );
                const float sx = ( ( px + pw * 0.5f ) - vw * 0.5f - ox );
                const float sy = ( ( py + ph ) - vh - oy );
                engine::IntRect src{};
                if ( auto* a = m_Player->Animator ( ) ) src = a->CurrentSrc ( );
                batch.Draw ( *tex , sx , sy ,
                             vw * game::SCALE , vh * game::SCALE ,
                             ( src.r > src.l ) ? &src : nullptr ,
                             engine::win32::RGBA8 ( 255 , 255 , 255 ) );
            }
        }

        // 3) Monsters
        for ( auto& m : m_Monsters ) if ( m && m->Alive ( ) ) {
            const auto* tex = m->TexturePtr ( );
            if ( !tex || !tex->srv ) continue;
            int mx , my , mw , mh; m->GetBounds ( mx , my , mw , mh );
            float vw , vh;         m->GetVisualSize ( vw , vh );
            const float sx = ( ( mx + mw * 0.5f ) - vw * 0.5f - ox );
            const float sy = ( ( my + mh ) - vh - oy );
            engine::IntRect src = m->SpriteSrc ( );
            batch.Draw ( *tex , sx , sy ,
                         vw * game::SCALE , vh * game::SCALE ,
                         &src , engine::win32::RGBA8 ( 255 , 255 , 255 ) );
        }
    }


    void PlaySession::RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh , bool drawEnabled ) {
        if ( !drawEnabled || !m_RenderSys ) return;

        auto* dbg = &m_RenderSys->Debug ( );

        dbg->WorldLine ( ox + 40.5f , oy + 40.5f ,
               ox + 300.5f , oy + 40.5f ,
               ox , oy , engine::win32::RGBA8 ( 255 , 0 , 0 , 255 ) );

        const int GRID = game::GRID_PX;
        const int wx0 = ox , wy0 = oy , wx1 = ox + sw , wy1 = oy + sh;
        int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
        for ( int x = gx; x <= wx1; x += GRID )
            dbg->WorldLine ( x , wy0 , x , wy1 , ox , oy , engine::win32::RGBA8 ( 60 , 60 , 60 ) );
        for ( int y = gy; y <= wy1; y += GRID )
            dbg->WorldLine ( wx0 , y , wx1 , y , ox , oy , engine::win32::RGBA8 ( 60 , 60 , 60 ) );

        // World colliders (via adapter)
        engine::physics::DebugDraw (
            m_World.Collision ( ) ,
            *dbg, ox , oy ,
            engine::win32::RGBA8 ( 255 , 60 , 60 ) ,    // solid
            engine::win32::RGBA8 ( 255 , 200 , 0 )     // oneway
        );

        // Player AABB
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            dbg->WorldRect ( px , py , pw , ph , ox , oy , engine::win32::RGBA8 ( 0 , 255 , 0 ) );
        }

        // Monsters debug (bounds + HP) via adapter
        for ( const auto& m : m_Monsters ) {
            if ( m && m->Alive ( ) ) {
                game::MonsterDebugDraw::Draw ( *m , dbg , ox , oy );
            }
        }

        // Projectile / HitVolume debug
        engine::D3D11DebugDrawAdapter idbg ( dbg );
        m_projSys.DebugDraw ( idbg , ox , oy );
        m_hitSys.DebugDraw ( idbg , ox , oy );

        // Doors
        for ( const auto& d : m_Doors ) {
            dbg->WorldRect ( d.x , d.y , d.w , d.h , ox , oy , engine::win32::RGBA8 ( 0 , 200 , 255 ) );
        }

        // Items (clear emblem etc.)
        for ( const auto& it : m_Items ) {
            if ( it.collected ) continue;

            dbg->WorldRect (
                it.x , it.y , it.w , it.h ,
                ox , oy ,
                engine::win32::RGBA8 ( 0 , 255 , 255 )
            );
        }

        // Boss arena AABB (magenta)
        if ( m_hasBossArena ) {
            const int w = m_bossArena.r - m_bossArena.l;
            const int h = m_bossArena.b - m_bossArena.t;
            if ( w > 0 && h > 0 ) {
                dbg->WorldRect ( m_bossArena.l , m_bossArena.t , w , h , ox , oy , engine::win32::RGBA8 ( 255 , 0 , 255 ) );
            }
        }
    }

    void PlaySession::RenderHUD ( int fps , double fixedDt ) {
        if ( !m_TextHUD ) return;
        m_TextHUD->Begin ( );

        // 1) FPS / dt
        wchar_t buf[ 128 ];
        std::swprintf ( buf , _countof ( buf ) , L"FPS:%d  dt:%.3f" , fps , fixedDt );
        m_TextHUD->DrawTextLine ( buf , 8.f , 8.f );

        // 2) State names (Move / Action / Overlay)
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
                        dbg.lastAABB.l , dbg.lastAABB.t ,
                        dbg.lastAABB.r , dbg.lastAABB.b , dbg.prevBottom );
        m_TextHUD->DrawTextLine ( line , 8.f , 88.f );

        // 4) Foot position / tile index
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

        // 5) Camera / monsters / HP
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
        std::swprintf ( hpLine , _countof ( hpLine ) ,
                        L"HP: %d  Lives: %d" ,
                        dbg.hp , Lives ( ) );
        m_TextHUD->DrawTextLine ( hpLine , 8.f , 168.f );


        // 6) Ability name
        const wchar_t* abilityName = L"None";
        switch ( dbg.ability ) {
        case game::Ability::Fire:  abilityName = L"Fire";  break;
        case game::Ability::Spark: abilityName = L"Spark"; break;
        case game::Ability::Beam:  abilityName = L"Beam";  break;
        default: break;
        }

        std::swprintf ( line , _countof ( line ) ,
                        L"Kirby  facing:%d  mouthFull:%d  ability:%ls" ,
                        dbg.facing , dbg.mouthFull ? 1 : 0 , abilityName );
        m_TextHUD->DrawTextLine ( line , 8.f , 188.f );

        std::swprintf ( line , _countof ( line ) ,
                        L"Action  inhaleT:%.2f  spitLockT:%.2f" ,
                        dbg.inhaleT , dbg.spitLockT );
        m_TextHUD->DrawTextLine ( line , 8.f , 208.f );

        // 7) Stage file (best-effort UTF-8 → wide naive)
        {
            wchar_t stage[ 256 ];
            std::swprintf ( stage , _countof ( stage ) , L"Stage: %hs" , m_stageJsonPath.c_str ( ) );
            m_TextHUD->DrawTextLine ( stage , 8.f , 228.f );
        }

        m_TextHUD->End ( );
    }

    void PlaySession::RenderOverlayFade ( int sw , int sh ) {
        if ( !m_RenderSys || !m_WhiteTex.srv ) return;
        m_fade.Render ( m_RenderSys , m_WhiteTex , sw , sh );
    }

    void PlaySession::RenderGameHUDSprites ( int sw , int sh )
    {
        if ( !m_RenderSys ) return;
        if ( !m_HudTex.srv ) return;

        auto& batch = m_RenderSys->Batch ( );
        const int scale = game::SCALE;

        // ==== 공통 상수 ====
        const int hudSrcH = 16;
        const int lifeIconW = 24;
        const int digitW = 8;
        const int hpCellW = 8;
        const int bossFrameW = 80;
        const int bossBarW = 80;

        // 화면 하단 기준 마진 (픽셀 단위, 스케일 적용된 좌표)
        const float marginBottom = 8.f * scale;
        const float marginSide = 8.f * scale;

        // -------------------------------------------------
        // 1) 커비 생명수 (중앙 하단)
        // -------------------------------------------------
        const int lives = std::clamp ( Lives ( ) , 0 , 99 );

        const int tens = lives / 10;
        const int ones = lives % 10;

        const float lifeIconDstW = lifeIconW * scale;
        const float lifeIconDstH = hudSrcH * scale;
        const float digitDstW = digitW * scale;
        const float digitDstH = hudSrcH * scale;

        // "아이콘 + 공백 + 2자리 숫자" 전체 폭
        const float gapPx = 4.f * scale; // 아이콘과 숫자 사이 간격
        const float totalW =
            lifeIconDstW + gapPx + digitDstW * 2.f;

        const float baseY = sh - marginBottom - lifeIconDstH;
        const float baseX = ( sw - totalW ) * 0.5f;

        // 1-1) 생명 아이콘 src rect: 0~23
        engine::IntRect srcLife{ 0, 0, lifeIconW, hudSrcH };
        batch.Draw (
            m_HudTex ,
            baseX , baseY ,
            lifeIconDstW , lifeIconDstH ,
            &srcLife ,
            engine::win32::RGBA8 ( 255 , 255 , 255 )
        );

        // 1-2) 숫자 rect helper
        auto digitSrcRect = [ ] ( int d ) -> engine::IntRect {
            const int x0 = 32 + d * 8;
            return engine::IntRect{ x0 , 0 , x0 + 8 , 16 };
            };

        const float digit0X = baseX + lifeIconDstW + gapPx;
        const float digit1X = digit0X + digitDstW;

        engine::IntRect srcTens = digitSrcRect ( tens );
        engine::IntRect srcOnes = digitSrcRect ( ones );

        batch.Draw (
            m_HudTex ,
            digit0X , baseY ,
            digitDstW , digitDstH ,
            &srcTens ,
            engine::win32::RGBA8 ( 255 , 255 , 255 )
        );
        batch.Draw (
            m_HudTex ,
            digit1X , baseY ,
            digitDstW , digitDstH ,
            &srcOnes ,
            engine::win32::RGBA8 ( 255 , 255 , 255 )
        );

        // -------------------------------------------------
        // 2) 플레이어 HP (커비 생명 위쪽에 6칸 나열)
        // -------------------------------------------------
        const int hp = std::clamp ( m_PlayerFSM.Hp ( ) , 0 , m_PlayerFSM.MaxHp ( ) );
        const int maxHp = m_PlayerFSM.MaxHp ( ); // 현재는 6

        engine::IntRect srcHpFull{ 112 , 0 , 120 , 16 }; // 112~119
        engine::IntRect srcHpEmpty{ 120 , 0 , 128 , 16 }; // 120~127

        const float hpCellDstW = hpCellW * scale;
        const float hpCellDstH = hudSrcH * scale;

        const float hpTotalW = hpCellDstW * maxHp;
        const float hpBaseX = ( sw - hpTotalW ) * 0.5f;
        const float hpBaseY = baseY - hpCellDstH - 4.f * scale; // 생명 표시 바로 위

        for ( int i = 0; i < maxHp; ++i ) {
            const bool filled = ( i < hp );
            const float x = hpBaseX + i * hpCellDstW;
            const float y = hpBaseY;

            const engine::IntRect& src = filled ? srcHpFull : srcHpEmpty;

            batch.Draw (
                m_HudTex ,
                x , y ,
                hpCellDstW , hpCellDstH ,
                &src ,
                engine::win32::RGBA8 ( 255 , 255 , 255 )
            );
        }

        // -------------------------------------------------
        // 3) 보스 HP (우측 하단)
        // -------------------------------------------------
        // 아직 실제 보스 HP 시스템이 없으면, 나중에
        // float ratio = BossHpRatio(); // 0~1
        // 같은 식으로 빼서 연결하면 됨.
        //
        // 여기 예시는 "보스가 있을 때만" 그린다고 가정하고,
        // 일단 isBossAlive()가 true일 때만 표시하도록 한다.
        if ( isBossAlive ( ) ) {
            const float ratio = 1.0f; // TODO: 실제 보스 HP / MaxHP 로 교체

            engine::IntRect srcFrame{ 128 , 0 , 128 + bossFrameW , 16 }; // 128~207
            engine::IntRect srcBarFull{ 208 , 0 , 208 + bossBarW , 16 }; // 208~287

            const float frameDstW = bossFrameW * scale;
            const float frameDstH = hudSrcH * scale;

            const float frameX = sw - marginSide - frameDstW;
            const float frameY = sh - marginBottom - frameDstH;

            // 3-1) 체력바: ratio만큼 잘라서 먼저 그림
            const int   barSrcFullW = srcBarFull.r - srcBarFull.l; // 80
            const int   barSrcW = static_cast< int >( std::round ( barSrcFullW * std::clamp ( ratio , 0.f , 1.f ) ) );
            const float barDstW = barSrcW * scale;
            const float barDstH = frameDstH;

            engine::IntRect srcBar{
                srcBarFull.l ,
                srcBarFull.t ,
                srcBarFull.l + barSrcW ,
                srcBarFull.b
            };

            // 단순히 프레임 안쪽 왼쪽에서부터 채운다고 가정
            const float barX = frameX;
            const float barY = frameY;

            batch.Draw (
                m_HudTex ,
                barX , barY ,
                barDstW , barDstH ,
                &srcBar ,
                engine::win32::RGBA8 ( 255 , 255 , 255 )
            );

            // 3-2) 프레임을 위에 덮어서 테두리 강조
            batch.Draw (
                m_HudTex ,
                frameX , frameY ,
                frameDstW , frameDstH ,
                &srcFrame ,
                engine::win32::RGBA8 ( 255 , 255 , 255 )
            );
        }
    }

    void PlaySession::RenderGameOverOverlay ( int sw , int sh )
    {
        if ( !m_RenderSys )      return;
        if ( !m_GameOverTex.srv ) return;

        // 게임오버 화면이 진행 중이거나, 이미 끝났지만 아직 세션이 살아있는 한은 계속 그려줘도 됨
        if ( !m_life.gameOverScreenActive && !m_life.gameOver )
            return;

        const float dstW = static_cast< float >( sw );
        const float dstH = static_cast< float >( sh );

        m_RenderSys->DrawSprite (
            m_GameOverTex ,
            0.f , 0.f ,
            dstW , dstH ,
            nullptr ,
            engine::win32::RGBA8 ( 255 , 255 , 255 ) ,
            0.f , 0.f , 0.f ,
            game::Z::OverlayTop ,          // 항상 최상단
            engine::BlendMode::Alpha ,
            engine::SamplerMode::Linear
        );
    }


} // namespace game
