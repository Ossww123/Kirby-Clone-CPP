// app/game/GameApp.h
//
// Role: App bootstrap + main loop (owns time/input/scene/render/session).
// Note: App-level wiring only (Win32/D3D11 + engine + game); no per-entity gameplay logic here.
//

#pragma once

#include <memory>
#include <cstdint>

// Win32 HWND forward decl to keep header light
struct HWND__;
using HWND = HWND__*;

// Engine core
#include "engine/core/RenderSystem.h"
#include "engine/save/SaveStorage.h"
#include "game/session/SessionState.h"

namespace engine {
    class Time;
    class Input;

    class IRenderer;
    class DWriteTextHUD;
} // namespace engine

namespace game { class PlaySession; class FrontFlow; }

namespace game {

    class GameApp {
    public:
        ~GameApp ( );

        void Init ( HWND hWnd );
        std::intptr_t OnWndMessage ( HWND hWnd , unsigned msg , std::uintptr_t wParam , std::intptr_t lParam );
        void OnResize ( int w , int h );
        bool DoOneFrame ( );

    private:
        void FixedUpdate ( double fixedDt );  // physics / gameplay tick
        void RenderFrame ( );                 // tile / player / HUD / debug

        void InitBindings ( );
        void InitRendererUI ( HWND hWnd , int w , int h );
        void BootFrontFlow ( );

        enum class AppMode { Front , Session };

    private:
        // Window/Core
        HWND   m_hWnd{};
        std::unique_ptr<engine::Time>  m_Time;
        std::unique_ptr<engine::Input> m_Input;

        // Rendering
        std::unique_ptr<engine::IRenderer>        m_Renderer;
        std::unique_ptr<engine::DWriteTextHUD>    m_TextHUD;
        engine::RenderSystem                      m_Render{};

        // Misc
        bool m_comInitialized = false;
        bool m_debugDrawEnabled = true;

        // Game
        AppMode m_mode = AppMode::Front;
        std::unique_ptr<game::FrontFlow>   m_Front;
        std::unique_ptr<game::PlaySession> m_Session;

        // Save/Session
        engine::SaveStorage m_Save;
        game::SessionState  m_State;
    };

} // namespace game
