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

namespace {

    // Clamp and bucketize 0..100 → {0,20,40,60,80,100} index(0..5)
    inline int ProgressBucketIndex ( int percent ) {
        static const int kSteps[ 6 ] = { 0,20,40,60,80,100 };
        percent = std::clamp ( percent , 0 , 100 );
        int idx = 0;
        for ( int i = 0; i < 6; ++i ) {
            if ( percent >= kSteps[ i ] ) idx = i;
        }
        return idx; // 마지막으로 만족한 스텝
    }
}


namespace game {

    void FrontFlow::Initialize ( const FrontFlowCreate& d ) {
        m_Text = d.text; m_Input = d.input; m_Save = d.save; m_State = d.state;
        m_Renderer = d.renderer; m_RenderSys = d.renderSys;
        m_sw = d.screenW; m_sh = d.screenH;
        m_focus = 0; m_slotFocus = 0; m_modeFocus = 0; m_navCd = 0.f; m_selectedSlot = 1;
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
            m_modeFocus = 0;
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

        // --- Title ---
        engine::LoadTextureWIC ( dev , L"assets/ui/title_background.png" , &m_titleBG );
        engine::LoadTextureWIC ( dev , L"assets/ui/title_logo.png" , &m_titleLogo );
        engine::CreateSolidTexture1x1 ( dev , 0xFFFFFFFFu , &m_whiteTex );

        game::LoadAnimCSV ( "assets/ui/title_logo.anim.csv" , &m_titleAnim , /*clear=*/true );
        if ( m_titleAnim.HasClip ( "IDLE" ) ) { m_titleAnim.Play ( "IDLE" , true ); }

        // --- Save Select background / slot focus ---
        engine::LoadTextureWIC ( dev , L"assets/ui/file_select_background.png" , &m_fileBG );

        engine::LoadTextureWIC ( dev , L"assets/ui/slot_1_focus.png" , &m_slotFocusTex[ 0 ] );
        engine::LoadTextureWIC ( dev , L"assets/ui/slot_2_focus.png" , &m_slotFocusTex[ 1 ] );
        engine::LoadTextureWIC ( dev , L"assets/ui/slot_3_focus.png" , &m_slotFocusTex[ 2 ] );

        // --- Save Select progress cards ---
        struct StepEntry { int percent; const wchar_t* suffix; };
        static const StepEntry kSteps[ 6 ] = {
            {  0,  L"0"   },
            { 20,  L"20"  },
            { 40,  L"40"  },
            { 60,  L"60"  },
            { 80,  L"80"  },
            { 100, L"100" },
        };

        for ( int i = 0; i < 6; ++i ) {
            const auto& s = kSteps[ i ];

            wchar_t pathN[ 256 ];
            wchar_t pathF[ 256 ];
            std::swprintf ( pathN , _countof ( pathN ) ,
                            L"assets/ui/file_select_%ls_normal.png" , s.suffix );
            std::swprintf ( pathF , _countof ( pathF ) ,
                            L"assets/ui/file_select_%ls_focus.png" , s.suffix );

            engine::LoadTextureWIC ( dev , pathN , &m_fileProgNormal[ i ] );
            engine::LoadTextureWIC ( dev , pathF , &m_fileProgFocus[ i ] );
        }

        // --- Save Select overlays (mode select UI) ---
        engine::LoadTextureWIC ( dev , L"assets/ui/file_select_overlay_solo.png" , &m_fileOverlaySolo );
        engine::LoadTextureWIC ( dev , L"assets/ui/file_select_overlay_multi.png" , &m_fileOverlayMulti );

        // 필수 최소 리소스만 체크 (나머지는 없으면 없는대로 처리)
        const bool titleOk =
            ( m_titleBG.srv && m_titleLogo.srv && m_whiteTex.srv );

        const bool saveOk =
            m_fileBG.srv != nullptr;

        return titleOk && saveOk;
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
        if ( !m_RenderSys ) {
            // 안전장치: 렌더러 없으면 기존 텍스트 버전 유지
            if ( !m_Text ) return;
            m_Text->Begin ( );
            m_Text->DrawTextLine ( L"[SAVE] reachable (no RenderSys)" , 8.f , 48.f );
            m_Text->End ( );
            return;
        }

        const int bbW = m_RenderSys->BackbufferWidth ( );
        const int bbH = m_RenderSys->BackbufferHeight ( );

        const float BASE_W = 240.f;
        const float BASE_H = 160.f;
        const float scale = std::max (
            1.0f ,
            std::floor ( std::min ( bbW / BASE_W , bbH / BASE_H ) )
        );

        const float drawW = BASE_W * scale;
        const float drawH = BASE_H * scale;
        const float baseX = ( bbW - drawW ) * 0.5f;
        const float baseY = ( bbH - drawH ) * 0.5f;

        // 1) Background (full screen)
        if ( m_fileBG.srv ) {
            m_RenderSys->DrawSprite (
                m_fileBG ,
                baseX , baseY , drawW , drawH ,
                /*src*/nullptr ,
                0xFFFFFFFFu ,
                0.f , 0.f , 0.f ,
                /*z*/ game::Z::BG ,
                engine::BlendMode::Alpha ,
                engine::SamplerMode::Linear
            );
        }

        // 각 슬롯의 세로 위치 (240x160 기준)
        const float slotBaseY[ 3 ] = {
            52.f,   // slot 1
            86.f,   // slot 2
            120.f   // slot 3
        };

        // 2) Slots: focus icon + progress card
        for ( int i = 0; i < 3; ++i ) {
            const auto& si = m_slots[ i ];
            const bool focused = ( i == m_slotFocus );

            // 진행도 → 버킷 index (0,20,40,60,80,100)
            const int prog = si.has ? protocol::ProgressT1 ( si.data ) : 0;
            const int stepIdx = ProgressBucketIndex ( prog );

            const engine::Tex2D& cardTex =
                focused ? m_fileProgFocus[ stepIdx ]
                : m_fileProgNormal[ stepIdx ];

            if ( !cardTex.srv ) continue;

            const float cardW = cardTex.width * scale;
            const float cardH = cardTex.height * scale;
            const float slotY = baseY + slotBaseY[ i ] * scale;

            // 카드 기본 X (중앙 정렬)
            float cardX = baseX + ( drawW - cardW ) * 0.5f;

            // 포커스인 경우 카드 살짝 오른쪽으로 밀기
            const float focusCardOffsetX = 6.f * scale;    // 느낌 안 맞으면 여기 숫자만 조절하면 됨
            if ( focused ) {
                cardX += focusCardOffsetX;
            }

            // 2-1) Focus icon (left of card, focused slot만)
            if ( focused ) {
                const engine::Tex2D& focusTex = m_slotFocusTex[ i ];
                if ( focusTex.srv ) {
                    const float fxW = focusTex.width * scale;
                    const float fxH = focusTex.height * scale;

                    // 카드 왼쪽에 살짝 띄우기
                    const float gap = 4.f * scale;
                    const float fxX = cardX - fxW - gap;
                    const float fxY = slotY + ( cardH - fxH ) * 0.5f;

                    m_RenderSys->DrawSprite (
                        focusTex ,
                        fxX , fxY , fxW , fxH ,
                        nullptr ,
                        0xFFFFFFFFu ,
                        0.f , 0.f , 0.f ,
                        /*z*/ game::Z::UIBase ,      // 배경 위, 카드와 비슷한 레이어
                        engine::BlendMode::Alpha ,
                        engine::SamplerMode::Point
                    );
                }
            }

            // 2-2) Progress card
            m_RenderSys->DrawSprite (
                cardTex ,
                cardX , slotY , cardW , cardH ,
                nullptr ,
                0xFFFFFFFFu ,
                0.f , 0.f , 0.f ,
                /*z*/ game::Z::UIBase + 10 ,       // 포커스 아이콘보다 살짝 위
                engine::BlendMode::Alpha ,
                engine::SamplerMode::Point
            );
        }

        // 3) 텍스트 보조 (디버그/설명용)
        if ( m_Text ) {
            m_Text->Begin ( );

            drawCenter ( L"Select Save Slot" , baseY + 16.f * scale );

            float yTxt = baseY + 40.f * scale;
            for ( int i = 0; i < 3; ++i ) {
                wchar_t line[ 256 ];
                const bool cur = ( i == m_slotFocus );
                const auto& s = m_slots[ i ];
                if ( !s.has ) {
                    std::swprintf (
                        line , _countof ( line ) ,
                        L"%lc Slot %d — Empty" ,
                        cur ? L'▶' : L' ' , i + 1
                    );
                }
                else {
                    const int prog = protocol::ProgressT1 ( s.data );
                    wchar_t last[ 128 ];
                    std::swprintf ( last , _countof ( last ) , L"%hs" , s.data.lastStage.c_str ( ) );
                    std::swprintf (
                        line , _countof ( line ) ,
                        L"%lc Slot %d — %d%%   Last: %ls" ,
                        cur ? L'▶' : L' ' , i + 1 , prog , last
                    );
                }
                m_Text->DrawTextLine ( line , baseX + 8.f * scale , yTxt );
                yTxt += 20.f * scale;
            }

            m_Text->DrawTextLine (
                L"Back: Backspace" ,
                baseX + 8.f * scale ,
                baseY + drawH - 24.f * scale
            );

            m_Text->End ( );
        }
    }



    // ---- ModeSelect ----
    void FrontFlow::updateMode ( double ) {
        if ( !m_Input ) return;

        const float ay = m_Input->GetAxis ( "MoveY" );
        if ( m_navCd <= 0.f ) {
            if ( ay < -0.5f ) { m_focus = ( m_focus + 2 ) % 3; m_modeFocus = ( m_modeFocus == 0 ) ? 1 : 0; m_navCd = 0.14f; }
            if ( ay > 0.5f ) { m_focus = ( m_focus + 1 ) % 3; m_modeFocus = ( m_modeFocus == 0 ) ? 1 : 0; m_navCd = 0.14f; }
        }
        if ( m_Input->ActionPressed ( "Back" ) ) { enter ( Screen::SaveSelect ); return; }

        if ( m_Input->ActionPressed ( "Confirm" ) ) {
            if ( m_focus == 0 ) { if ( onStartSolo ) onStartSolo ( m_selectedSlot ); }
            else if ( m_focus == 1 ) { /* Co-op (coming soon) */ }
            else { enter ( Screen::SaveSelect ); }
        }
    }

    void FrontFlow::renderMode ( ) {
        if ( !m_RenderSys ) {
            // 안전장치: 텍스트 폴백만
            if ( !m_Text ) return;
            m_Text->Begin ( );
            drawCenter ( L"Select Mode (Solo/Multi overlay missing)" , m_sh * 0.22f );
            m_Text->End ( );
            return;
        }

        // 0) 먼저 Save 화면을 그대로 그린다 (배경 + 슬롯 카드 + 텍스트)
        renderSave ( );

        // 1) 오버레이 텍스처 선택 (solo / multi)
        const engine::Tex2D* ovTex =
            ( m_modeFocus == 0 ) ? &m_fileOverlaySolo : &m_fileOverlayMulti;

        if ( !ovTex || !ovTex->srv ) return;

        const int bbW = m_RenderSys->BackbufferWidth ( );
        const int bbH = m_RenderSys->BackbufferHeight ( );

        const float BASE_W = 240.f;
        const float BASE_H = 160.f;
        const float scale = std::max (
            1.0f ,
            std::floor ( std::min ( bbW / BASE_W , bbH / BASE_H ) )
        );

        const float drawW = BASE_W * scale;
        const float drawH = BASE_H * scale;
        const float baseX = ( bbW - drawW ) * 0.5f;
        const float baseY = ( bbH - drawH ) * 0.5f;

        const float ovW = ovTex->width * scale;
        const float ovH = ovTex->height * scale;

        // 일단 중앙 근처에 띄우기 (수치는 나중에 직접 조정)
        const float ovX = baseX + ( drawW - ovW ) * 0.5f;
        const float ovY = baseY + ( drawH - ovH ) * 0.5f;

        m_RenderSys->DrawSprite (
            *ovTex ,
            ovX , ovY , ovW , ovH ,
            nullptr ,
            0xFFFFFFFFu ,
            0.f , 0.f , 0.f ,
            /*z*/ game::Z::OverlayTop ,         // 슬롯/텍스트 위 최상단
            engine::BlendMode::Alpha ,
            engine::SamplerMode::Point
        );

        // 텍스트 안내 정도는 남겨둘 수 있음 (옵션)
        if ( m_Text ) {
            m_Text->Begin ( );
            m_Text->DrawTextLine (
                ( m_modeFocus == 0 ) ? L"Solo" : L"Multi" ,
                ovX + 8.f * scale ,
                ovY + ovH + 8.f * scale
            );
            m_Text->DrawTextLine (
                L"↑/↓: Change   Enter: Confirm   Backspace: Back" ,
                baseX + 8.f * scale ,
                baseY + drawH - 20.f * scale
            );
            m_Text->End ( );
        }
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
