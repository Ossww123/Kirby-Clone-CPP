#include "game/frontend/FrontFlow.h"
#include <cwchar>
#include <algorithm>

// deps
#include "engine/render/DWriteText.h"
#include "engine/core/Input.h"
#include "engine/save/SaveStorage.h"
#include "game/session/SessionState.h"

#ifndef DBGLOG
#include <string>
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) { ::OutputDebugStringW ( msg ); ::OutputDebugStringW ( L"\n" ); }
#endif

namespace game {

    void FrontFlow::Initialize ( const FrontFlowCreate& d ) {
        m_Renderer = d.renderer; m_Batch = d.batch; m_Text = d.text;
        m_Input = d.input; m_Save = d.save; m_State = d.state;
        m_sw = d.screenW; m_sh = d.screenH;
        m_focus = 0; m_slotFocus = 0; m_navCd = 0.f; m_selectedSlot = 1;
        refreshSlotInfos ( );
        enter ( Screen::Title );
    }

    void FrontFlow::OnResize ( int w , int h ) { m_sw = w; m_sh = h; }

    void FrontFlow::Update ( double fixedDt ) {
        m_navCd = std::max ( 0.f , m_navCd - static_cast< float >( fixedDt ) );
        switch ( m_scr ) {
        case Screen::Title:      updateTitle ( fixedDt ); break;
        case Screen::SaveSelect: updateSave ( fixedDt );  break;
        case Screen::ModeSelect: updateMode ( fixedDt );  break;
        }
    }

    void FrontFlow::Render ( ) {
        DBGLOG ( L"[FrontFlow] Render()" );
        if ( !m_Text ) { DBGLOG ( L"[FrontFlow] m_Text is null" ); return; }

        m_Text->Begin ( );
        m_Text->DrawTextLine ( L"[FrontFlow] Render() watermark" , 8.f , 28.f ); // <— 확실히 보이는 라벨

        switch ( m_scr ) {
        case Screen::Title:      renderTitle ( ); break;
        case Screen::SaveSelect: renderSave ( );  break;
        case Screen::ModeSelect: renderMode ( );  break;
        }
        m_Text->End ( );
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
    void FrontFlow::updateTitle ( double ) {
        if ( m_Input && m_Input->ActionPressed ( "Confirm" ) ) {
            enter ( Screen::SaveSelect );
        }
    }
    void FrontFlow::renderTitle ( ) {
        m_Text->DrawTextLine ( L"[TITLE] reachable" , 8.f , 48.f );
        drawCenter ( L"KIRBY — work-in-progress" , m_sh * 0.40f );
        drawCenter ( L"Press Confirm" , m_sh * 0.60f );
    }

    // ---- SaveSelect ----
    void FrontFlow::updateSave ( double ) {
        if ( !m_Input ) return;

        // axis edge with small cooldown
        const float ay = m_Input->GetAxis ( "MoveY" );
        if ( m_navCd <= 0.f ) {
            if ( ay < -0.5f ) { m_slotFocus = ( m_slotFocus + 2 ) % 3; m_navCd = 0.14f; }
            if ( ay > 0.5f ) { m_slotFocus = ( m_slotFocus + 1 ) % 3; m_navCd = 0.14f; }
        }

        if ( m_Input->ActionPressed ( "Back" ) ) {
            enter ( Screen::Title );
            return;
        }

        if ( m_Input->ActionPressed ( "Confirm" ) ) {
            const int slot = m_slotFocus + 1;
            m_selectedSlot = slot;

            // 빈 슬롯이면 기본 세이브 생성 & 저장
            if ( !m_slots[ m_slotFocus ].has && m_Save ) {
                protocol::SaveData def{};
                def.lastHub = "t1/hub";
                def.lastStage = "t1/hub";
                def.lastSpawn = "default";
                m_Save->Save ( slot , def );
                // 즉시 캐시 갱신
                m_slots[ m_slotFocus ].has = true;
                m_slots[ m_slotFocus ].data = def;
            }
            enter ( Screen::ModeSelect );
        }
    }

    void FrontFlow::renderSave ( ) {
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
                drawLine ( line , x , y );
            }
            else {
                const int prog = protocol::ProgressT1 ( s.data );
                wchar_t last[ 128 ]; std::swprintf ( last , _countof ( last ) , L"%hs" , s.data.lastStage.c_str ( ) );
                std::swprintf ( line , _countof ( line ) , L"%lc [%d] Slot %d — %d%%   Last: %ls" ,
                              cur ? L'▶' : L' ' , i + 1 , i + 1 , prog , last );
                drawLine ( line , x , y );
            }
            y += 36.f;
        }
        drawLine ( L"Back: Backspace" , x , y + 12.f );
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
            if ( m_focus == 0 ) { // Solo
                if ( onStartSolo ) onStartSolo ( m_selectedSlot );
            }
            else if ( m_focus == 1 ) {
                // Co-op (coming soon) — intentionally no-op
            }
            else {
                enter ( Screen::SaveSelect );
            }
        }
    }

    void FrontFlow::renderMode ( ) {
        drawCenter ( L"Select Mode" , m_sh * 0.22f );

        const wchar_t* items[ 3 ] = { L"Solo", L"Co-op (coming soon)", L"Back" };
        const float x = 120.f;
        float y = m_sh * 0.40f;
        for ( int i = 0; i < 3; ++i ) {
            wchar_t line[ 256 ];
            std::swprintf ( line , _countof ( line ) , L"%lc %ls" , ( i == m_focus ) ? L'▶' : L' ' , items[ i ] );
            drawLine ( line , x , y );
            y += 36.f;
        }
    }

    // ---- draw helpers ----
    void FrontFlow::drawCenter ( const wchar_t* text , float y ) {
        if ( !m_Text || !text ) return;
        // 간단: 좌측 여백만 고정 오프셋, 중앙 정렬 대신 위치 보정
        m_Text->DrawTextLine ( text , std::max ( 8.f , ( m_sw - 640 ) * 0.5f + 40.f ) , y );
    }
    void FrontFlow::drawLine ( const wchar_t* text , float x , float y ) {
        if ( !m_Text || !text ) return;
        m_Text->DrawTextLine ( text , x , y );
    }

} // namespace game
