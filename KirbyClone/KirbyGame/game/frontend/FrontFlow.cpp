#include "game/frontend/FrontFlow.h"
#include <cwchar>
#include <algorithm>
#include <cmath>

// deps
#include "engine/render/DWriteText.h"
#include "engine/core/Input.h"
#include "engine/save/SaveStorage.h"
#include "engine/core/RenderSystem.h"           // RenderSystem::DrawSprite
#include "engine/render/Texture.h"              // LoadTextureWIC / CreateSolidTexture1x1
#include "engine/render/D3D11SpriteBatch.h"     // BlendMode / SamplerMode enum definitions
#include "engine/platform/win32/ColorUtil.h"    // engine::win32::RGBA8
#include "engine/render/TextureLoader.h"
#include "game/data/AnimCSV.h"                  // game::LoadAnimCSV
#include "game/session/SessionState.h"

#ifndef DBGLOG
#include <string>
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) { ::OutputDebugStringW ( msg ); ::OutputDebugStringW ( L"\n" ); }
#endif

namespace game {

    void FrontFlow::Initialize ( const FrontFlowCreate& d ) {
        m_Text = d.text; m_Input = d.input; m_Save = d.save; m_State = d.state;
        m_Renderer = d.renderer; m_RenderSys = d.renderSys;
        m_sw = d.screenW; m_sh = d.screenH;
        m_focus = 0; m_slotFocus = 0; m_navCd = 0.f; m_selectedSlot = 1;
        m_time = 0.f; m_fade = {};

        refreshSlotInfos ( );

        // --- 타이틀 리소스 로드 ---
        loadTitleAssets ( );

        enter ( Screen::Title );
        startFadeIn ( 0.6f , 0xFFFFFFu , game::Z::OverlayTop );
    }

    void FrontFlow::OnResize ( int w , int h ) { m_sw = w; m_sh = h; }

    void FrontFlow::Update ( double fixedDt ) {
        m_navCd = std::max ( 0.f , m_navCd - static_cast< float >( fixedDt ) );
        m_fade.Update ( fixedDt );
        m_time += static_cast< float >( fixedDt );

        // 타이틀에서 Confirm → 페이드아웃 시작 (중복 방지: 이미 페이드 중이면 무시)
        if ( m_scr == Screen::Title && m_Input && m_Input->ActionPressed ( "Confirm" ) ) {
            if ( !m_fade.Active ( ) ) {
                startFadeOut ( 0.6f , 0xFFFFFFu , game::Z::OverlayTop );
                m_waitTitleToSave = true;
            }
        }

        // 페이드아웃 완료 시점에 화면 전환
        if ( m_waitTitleToSave && !m_fade.Active ( ) ) {
            enter ( Screen::SaveSelect );
            startFadeIn ( 0.6f , 0xFFFFFFu , game::Z::OverlayTop );
            m_waitTitleToSave = false;
        }

        switch ( m_scr ) {
            case Screen::Title:      updateTitle ( fixedDt ); break;
            case Screen::SaveSelect: updateSave ( fixedDt );  break;
            case Screen::ModeSelect: updateMode ( fixedDt );  break;
        }
    }

    void FrontFlow::Render ( ) {
        if ( !m_RenderSys ) {
            // 텍스트 폴백
            DBGLOG ( L"[FrontFlow] Render() sprite path missing; text-only fallback" );
            if ( !m_Text ) return;
            m_Text->Begin ( );
            m_Text->DrawTextLine ( L"[FrontFlow] Render() watermark" , 8.f , 28.f );
            switch ( m_scr ) {
            case Screen::Title:      renderTitle ( ); break;
            case Screen::SaveSelect: renderSave ( );  break;
            case Screen::ModeSelect: renderMode ( );  break;
            }
            m_Text->End ( );
            return;
        }

        // 스프라이트 렌더
        switch ( m_scr ) {
        case Screen::Title:      renderTitle ( ); break;
        case Screen::SaveSelect: renderSave ( );  break;
        case Screen::ModeSelect: renderMode ( );  break;
        }

        renderFade ( );

        // 디버그 텍스트(있으면)
        if ( m_Text ) {
            m_Text->Begin ( );
            m_Text->DrawTextLine ( L"[FrontFlow] Render() watermark" , 8.f , 28.f );
            m_Text->End ( );
        }
    }

    // ===== internals =====
    void FrontFlow::enter ( Screen s ) {
        m_scr = s;
        if ( s == Screen::SaveSelect ) {
            refreshSlotInfos ( );
            m_slotFocus = std::clamp ( m_selectedSlot - 1 , 0 , 2 );
        }
        else if ( s == Screen::ModeSelect ) {
            m_focus = 0;
        }
    }

    void FrontFlow::refreshSlotInfos ( ) {
        if ( !m_Save ) return;
        for ( int i = 0; i < 3; ++i ) {
            SlotInfo info{};
            protocol::SaveData tmp{};
            info.has = m_Save->Load ( i + 1 , tmp );
            if ( info.has ) info.data = tmp;
            m_slots[ i ] = info;
        }
    }

    // ---- Title ----
    bool FrontFlow::loadTitleAssets ( ) {
        if ( !m_Renderer ) return false;

        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        if ( !m_Renderer->GetD3D11Handles ( &dev , &ctx ) ) return false;

        engine::LoadTextureWIC ( dev , L"assets/ui/title_background.png" , &m_titleBG );
        engine::LoadTextureWIC ( dev , L"assets/ui/title_logo.png" , &m_titleLogo );
        engine::CreateSolidTexture1x1 ( dev , 0xFFFFFFFFu , &m_whiteTex );

        game::LoadAnimCSV ( "assets/ui/title_logo.anim.csv" , &m_titleAnim , /*clear=*/true );
        if ( m_titleAnim.HasClip ( "IDLE" ) ) { m_titleAnim.Play ( "IDLE" , true ); }

        return ( m_titleBG.srv && m_titleLogo.srv && m_whiteTex.srv );
    }

    void FrontFlow::renderFade ( ) {
        if ( !m_RenderSys ) return;
        const int bbW = m_RenderSys->BackbufferWidth ( );
        const int bbH = m_RenderSys->BackbufferHeight ( );
        m_fade.Render ( m_RenderSys , m_whiteTex , bbW , bbH );
    }

    void FrontFlow::startFadeIn ( float sec , uint32_t rgb , int16_t z ) {
        game::Fade2D::Params p; p.duration = sec; p.rgb = rgb; p.z = z;
        m_fade.StartIn ( p );
    }

    void FrontFlow::startFadeOut ( float sec , uint32_t rgb , int16_t z ) {
        game::Fade2D::Params p; p.duration = sec; p.rgb = rgb; p.z = z;
        m_fade.StartOut ( p );
    }


    void FrontFlow::updateTitle ( double dt ) {
        m_titleAnim.Update ( dt );
    }

    void FrontFlow::renderTitle ( ) {
        const int bbW = m_RenderSys ? m_RenderSys->BackbufferWidth ( ) : m_sw;
        const int bbH = m_RenderSys ? m_RenderSys->BackbufferHeight ( ) : m_sh;

        // 1) Background (full screen)
        if ( m_titleBG.srv ) {
            m_RenderSys->DrawSprite (
                m_titleBG ,
                0.f , 0.f ,
                ( float ) bbW , ( float ) bbH ,
                /*src*/nullptr ,
                /*tint*/0xFFFFFFFF ,
                0.f , 0.f , 0.f ,
                /*zSort*/ -20000 ,                 // backmost
                engine::BlendMode::Alpha ,
                engine::SamplerMode::Linear
            );
        }

        // 2) Logo (animated frame) — integer scale from 240x160 base
        if ( m_titleLogo.srv ) {
            const engine::IntRect& src = m_titleAnim.CurrentSrc ( );
            const int frameW = src.r - src.l;
            const int frameH = src.b - src.t;

            const float scale = std::max ( 1.0f , std::floor ( std::min ( bbW / 240.f , bbH / 160.f ) ) );
            const float drawW = frameW * scale;
            const float drawH = frameH * scale;
            const float x = ( bbW - drawW ) * 0.5f;
            const float y = 16.f * scale;

            m_RenderSys->DrawSprite (
                m_titleLogo ,
                x , y , drawW , drawH ,
                &src ,
                0xFFFFFFFF ,
                0.f , 0.f , 0.f ,
                /*zSort*/ -19000 ,                 // above bg
                engine::BlendMode::Alpha ,
                engine::SamplerMode::Point        // pixel perfect
            );
        }

        // 3) "Press Enter" temporary text (keep as-is with HUD)
        if ( m_Text ) {
            m_Text->Begin ( );
            m_Text->DrawTextLine ( L"Press Enter" , std::max ( 8.f , ( bbW - 640 ) * 0.5f + 40.f ) , bbH * 0.75f );
            m_Text->End ( );
        }
    }


    // ---- SaveSelect ---- (스프라이트 적용은 다음 단계에서 확장)
    void FrontFlow::updateSave ( double ) {
        if ( !m_Input ) return;

        const float ay = m_Input->GetAxis ( "MoveY" );
        if ( m_navCd <= 0.f ) {
            if ( ay < -0.5f ) { m_slotFocus = ( m_slotFocus + 2 ) % 3; m_navCd = 0.14f; }
            if ( ay > 0.5f ) { m_slotFocus = ( m_slotFocus + 1 ) % 3; m_navCd = 0.14f; }
        }

        if ( m_Input->ActionPressed ( "Back" ) ) {
            enter ( Screen::Title );
            startFadeIn ( 0.3f );
            return;
        }

        if ( m_Input->ActionPressed ( "Confirm" ) ) {
            const int slot = m_slotFocus + 1;
            m_selectedSlot = slot;

            if ( !m_slots[ m_slotFocus ].has && m_Save ) {
                protocol::SaveData def{};
                def.lastHub = "t1/hub";
                def.lastStage = "t1/hub";
                def.lastSpawn = "default";
                m_Save->Save ( slot , def );
                m_slots[ m_slotFocus ].has = true;
                m_slots[ m_slotFocus ].data = def;
            }
            enter ( Screen::ModeSelect );
        }
    }

    void FrontFlow::renderSave ( ) {
        // 아직은 기존 텍스트 버전 유지(다음 단계에서 스프라이트 교체)
        if ( !m_Text ) return;
        m_Text->Begin ( );
        m_Text->DrawTextLine ( L"[SAVE] reachable" , 8.f , 48.f );

        drawCenter ( L"Select Save Slot" , m_sh * 0.22f );
        const float x = 80.f;
        float y = m_sh * 0.35f;
        for ( int i = 0; i < 3; ++i ) {
            wchar_t line[ 256 ];
            const bool cur = ( i == m_slotFocus );
            const auto& s = m_slots[ i ];
            if ( !s.has ) {
                std::swprintf ( line , _countof ( line ) , L"%lc [%d] Slot %d — Empty" ,
                              cur ? L'▶' : L' ' , i + 1 , i + 1 );
                m_Text->DrawTextLine ( line , x , y );
            }
            else {
                const int prog = protocol::ProgressT1 ( s.data );
                wchar_t last[ 128 ]; std::swprintf ( last , _countof ( last ) , L"%hs" , s.data.lastStage.c_str ( ) );
                std::swprintf ( line , _countof ( line ) , L"%lc [%d] Slot %d — %d%%   Last: %ls" ,
                              cur ? L'▶' : L' ' , i + 1 , i + 1 , prog , last );
                m_Text->DrawTextLine ( line , x , y );
            }
            y += 36.f;
        }
        m_Text->DrawTextLine ( L"Back: Backspace" , x , y + 12.f );
        m_Text->End ( );
    }

    // ---- ModeSelect ----
    void FrontFlow::updateMode ( double ) {
        if ( !m_Input ) return;

        const float ay = m_Input->GetAxis ( "MoveY" );
        if ( m_navCd <= 0.f ) {
            if ( ay < -0.5f ) { m_focus = ( m_focus + 2 ) % 3; m_navCd = 0.14f; }
            if ( ay > 0.5f ) { m_focus = ( m_focus + 1 ) % 3; m_navCd = 0.14f; }
        }
        if ( m_Input->ActionPressed ( "Back" ) ) { enter ( Screen::SaveSelect ); return; }

        if ( m_Input->ActionPressed ( "Confirm" ) ) {
            if ( m_focus == 0 ) { if ( onStartSolo ) onStartSolo ( m_selectedSlot ); }
            else if ( m_focus == 1 ) { /* Co-op (coming soon) */ }
            else { enter ( Screen::SaveSelect ); }
        }
    }

    void FrontFlow::renderMode ( ) {
        if ( !m_Text ) return;
        m_Text->Begin ( );
        drawCenter ( L"Select Mode" , m_sh * 0.22f );

        const wchar_t* items[ 3 ] = { L"Solo", L"Co-op (coming soon)", L"Back" };
        const float x = 120.f;
        float y = m_sh * 0.40f;
        for ( int i = 0; i < 3; ++i ) {
            wchar_t line[ 256 ];
            std::swprintf ( line , _countof ( line ) , L"%lc %ls" , ( i == m_focus ) ? L'▶' : L' ' , items[ i ] );
            m_Text->DrawTextLine ( line , x , y );
            y += 36.f;
        }
        m_Text->End ( );
    }

    // ---- draw helpers ----
    void FrontFlow::drawCenter ( const wchar_t* text , float y ) {
        if ( !m_Text || !text ) return;
        const int bbW = m_RenderSys ? m_RenderSys->BackbufferWidth ( ) : m_sw;
        m_Text->DrawTextLine ( text , std::max ( 8.f , ( bbW - 640 ) * 0.5f + 40.f ) , y );
    }
    void FrontFlow::drawLine ( const wchar_t* text , float x , float y ) {
        if ( !m_Text || !text ) return;
        m_Text->DrawTextLine ( text , x , y );
    }

} // namespace game
