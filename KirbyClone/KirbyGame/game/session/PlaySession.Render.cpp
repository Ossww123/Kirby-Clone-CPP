// PlaySession.Render.cpp
//
// Responsibility: Session render paths (parallax background, world, debug, HUD, fade).
// Non-Goals    : Resource loading policy or scene ownership.
// Call-Context : Called by GameApp between Begin/EndFrame.

#include "game/session/PlaySession.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/render/D3D11DebugDraw.h"
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

        m_Batch->Draw ( m_BgTex , 0.f , 0.f , ( float ) sw , ( float ) sh , &src ,
                        engine::win32::RGBA8 ( 255 , 255 , 255 ) );
    }

    void PlaySession::RenderWorld ( int ox , int oy , int sw , int sh ) {
        // 1) Tiles
        m_World.RenderVisible ( *m_Batch , ox , oy , sw , sh );

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
                m_Batch->Draw ( *tex , sx , sy , vw * game::SCALE , vh * game::SCALE ,
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
            m_Batch->Draw ( *tex , sx , sy , vw * game::SCALE , vh * game::SCALE ,
                            &src , engine::win32::RGBA8 ( 255 , 255 , 255 ) );
        }
    }

    void PlaySession::RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh , bool drawEnabled ) {
        if ( !drawEnabled || !m_Debug ) return;

        const int GRID = game::GRID_PX;
        const int wx0 = ox , wy0 = oy , wx1 = ox + sw , wy1 = oy + sh;
        int gx = ( wx0 / GRID ) * GRID , gy = ( wy0 / GRID ) * GRID;
        for ( int x = gx; x <= wx1; x += GRID )
            m_Debug->WorldLine ( x , wy0 , x , wy1 , ox , oy , engine::win32::RGBA8 ( 60 , 60 , 60 ) );
        for ( int y = gy; y <= wy1; y += GRID )
            m_Debug->WorldLine ( wx0 , y , wx1 , y , ox , oy , engine::win32::RGBA8 ( 60 , 60 , 60 ) );

        // World colliders (via adapter)
        engine::physics::DebugDraw (
            m_World.Collision ( ) ,
            *m_Debug ,
            ox , oy ,
            engine::win32::RGBA8 ( 255 , 60 , 60 ) ,   // solid
            engine::win32::RGBA8 ( 255 , 200 , 0 )    // oneway
        );

        // Player AABB
        if ( m_Player ) {
            int px , py , pw , ph; m_Player->GetBounds ( px , py , pw , ph );
            m_Debug->WorldRect ( px , py , pw , ph , ox , oy , engine::win32::RGBA8 ( 0 , 255 , 0 ) );
        }

        // Monsters debug (bounds + HP) via adapter
        for ( const auto& m : m_Monsters ) {
            if ( m && m->Alive ( ) ) {
                game::MonsterDebugDraw::Draw ( *m , m_Debug , ox , oy );
            }
        }

        // Projectile / HitVolume debug
        m_projSys.DebugDraw ( *m_Debug , ox , oy );
        m_hitSys.DebugDraw ( *m_Debug , ox , oy );

        // Doors
        for ( const auto& d : m_Doors ) {
            m_Debug->WorldRect ( d.x , d.y , d.w , d.h , ox , oy , engine::win32::RGBA8 ( 0 , 200 , 255 ) );
        }

        // Boss arena AABB (magenta)
        if ( m_hasBossArena ) {
            const int w = m_bossArena.r - m_bossArena.l;
            const int h = m_bossArena.b - m_bossArena.t;
            if ( w > 0 && h > 0 ) {
                m_Debug->WorldRect ( m_bossArena.l , m_bossArena.t , w , h , ox , oy ,
                                     engine::win32::RGBA8 ( 255 , 0 , 255 ) );
            }
        }

        m_Debug->Flush ( );
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
        std::swprintf ( hpLine , _countof ( hpLine ) , L"HP: %d" , dbg.hp );
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
        if ( !m_Batch || !m_WhiteTex.srv ) return;
        if ( m_fade.mode == Fade::None || m_fade.dur <= 0.f ) return;

        const float t = std::clamp ( m_fade.t / std::max ( 0.0001f , m_fade.dur ) , 0.f , 1.f );
        const float alpha = ( m_fade.mode == Fade::Out ) ? t : ( 1.f - t ); // Out: 0→1, In: 1→0
        const uint8_t a = ( uint8_t ) std::lround ( alpha * 255.f );

        const uint8_t r = ( uint8_t ) ( ( m_fade.rgb >> 16 ) & 0xFF );
        const uint8_t g = ( uint8_t ) ( ( m_fade.rgb >> 8 ) & 0xFF );
        const uint8_t b = ( uint8_t ) ( m_fade.rgb & 0xFF );

        m_Batch->Draw ( m_WhiteTex , 0.f , 0.f , ( float ) sw , ( float ) sh , nullptr ,
                        engine::win32::RGBA8 ( r , g , b , a ) );
    }

} // namespace game
