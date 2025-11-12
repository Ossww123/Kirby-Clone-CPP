#pragma once
//
// Responsibility: App bootstrap + main loop. Owns input/time/scene and wires renderer & session.
// Non-Goals:      Direct gameplay logic or low-level D3D state; those live in subsystems.
// Call-Context:   Win32 windowed app (single-threaded game loop).
//

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
    class Scene;

    class IRenderer;
    class D3D11SpriteBatch;
    class D3D11DebugDraw;
    class DWriteTextHUD;
} // namespace engine

namespace game { class PlaySession; class FrontFlow; }

namespace engine {

    class GameApp {
    public:
        ~GameApp ( );

        void Init ( HWND hWnd );
        std::intptr_t OnWndMessage ( HWND hWnd , unsigned msg ,
                                    std::uintptr_t wParam , std::intptr_t lParam );
        void OnResize ( int w , int h );
        bool DoOneFrame ( );

    private:
        void FixedUpdate ( double fixedDt );  // physics / gameplay tick
        void RenderFrame ( );                 // tile / player / HUD / debug
        void InitBindings ( );
        void InitRendererUI ( HWND hWnd , int w , int h );

        enum class AppMode { Front , Session };

    private:
        // Window/Core
        HWND   m_hWnd{};
        std::unique_ptr<Time>  m_Time;
        std::unique_ptr<Input> m_Input;
        std::unique_ptr<Scene> m_Scene;

        // Rendering
        std::unique_ptr<IRenderer>        m_Renderer;
        std::unique_ptr<D3D11SpriteBatch> m_Batch;
        std::unique_ptr<D3D11DebugDraw>   m_Debug;
        std::unique_ptr<DWriteTextHUD>    m_TextHUD;
        RenderSystem                      m_Render{};

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

} // namespace engine
