#pragma once
//
// Responsibility: Front-end flow (Title → SaveSelect → ModeSelect) before gameplay session.
// Non-Goals:      Gameplay/world/physics; async IO; fancy UI widgets.
// Call-Context:   Main thread; driven by GameApp fixed-step and rendered with DWriteTextHUD.
//
#include <functional>
#include <array>
#include <string>
#include <algorithm>
#include "protocol/SaveSchema.h"
#include "game/effects/Fade2D.h"
#include "engine/render/Texture.h"
#include "engine/util/Anim.h"

namespace engine { 
    class DWriteTextHUD; class Input; class SaveStorage;
    class IRenderer; class RenderSystem;
}

namespace game {
    class SessionState;

    struct FrontFlowCreate {
        engine::DWriteTextHUD*  text{};
        engine::Input*          input{};
        engine::SaveStorage*    save{};
        SessionState*           state{};
        int                     screenW{} , screenH{};
        engine::IRenderer*      renderer{};
        engine::RenderSystem*   renderSys{};
    };

    class FrontFlow {
    public:
        void Initialize ( const FrontFlowCreate& d );
        void OnResize ( int w , int h );
        void Update ( double fixedDt );
        void Render ( );

        // GameApp sets this to transition into gameplay
        std::function<void ( int /*slot 1..3*/ )> onStartSolo;

    private:
        enum class Screen { Title , SaveSelect , ModeSelect } m_scr{ Screen::Title };
        enum class FadeLayer { OverlayTop , UnderLogo };

        struct SlotInfo {
            bool has{ false };
            protocol::SaveData data{};
        };

        // deps
        engine::DWriteTextHUD*  m_Text{};
        engine::Input*          m_Input{};
        engine::SaveStorage*    m_Save{};
        game::SessionState*     m_State{};
        engine::IRenderer*      m_Renderer{};
        engine::RenderSystem*   m_RenderSys{};
        int                     m_sw{} , m_sh{};

        // ui state
        int   m_focus{ 0 };        // generic focus (ModeSelect)
        int   m_slotFocus{ 0 };    // 0..2
        float m_navCd{ 0.f };      // navigation cooldown for axis edge
        int   m_selectedSlot{ 1 }; // last picked slot (1..3)

        std::array<SlotInfo , 3> m_slots{};

        // title resources/state
        engine::Tex2D       m_titleBG{};
        engine::Tex2D       m_titleLogo{};
        engine::Tex2D       m_whiteTex{};
        engine::Animator    m_titleAnim{};
        game::Fade2D        m_fade;
        float               m_time{ 0.f };
        bool                m_waitTitleToSave{ false };

    private:
        void enter ( Screen s );
        void refreshSlotInfos ( );
        void updateTitle ( double dt );
        void updateSave ( double dt );
        void updateMode ( double dt );
        void renderTitle ( );
        void renderSave ( );
        void renderMode ( );

        // small utilities
        void drawCenter ( const wchar_t* text , float y );
        void drawLine ( const wchar_t* text , float x , float y );

        // === utils ===
        bool loadTitleAssets ( );
        void renderFade ( );
        void startFadeIn ( float sec , uint32_t rgb = 0xFFFFFFu , int16_t z = game::Z::OverlayTop );
        void startFadeOut ( float sec , uint32_t rgb = 0xFFFFFFu , int16_t z = game::Z::OverlayTop );
    };

} // namespace game
