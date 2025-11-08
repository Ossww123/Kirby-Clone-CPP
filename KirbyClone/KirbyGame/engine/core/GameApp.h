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

namespace engine {

    // fwd (pointers only in this header)
    class Time;
    class Input;
    class Scene;

    class IRenderer;
    class D3D11SpriteBatch;
    class D3D11DebugDraw;
    class DWriteTextHUD;

} // namespace engine

namespace game { class PlaySession; }

namespace engine {

    class GameApp {
    public:
        ~GameApp ( );

        void Init ( HWND hWnd );
        std::intptr_t OnWndMessage ( HWND hWnd ,
                                    unsigned msg ,
                                    std::uintptr_t wParam ,
                                    std::intptr_t lParam );

        void OnResize ( int w , int h );
        bool DoOneFrame ( );

    private:
        void FixedUpdate ( double fixedDt );  // physics / gameplay tick
        void RenderFrame ( );                // tile / player / HUD / debug

        // init helpers
        void InitBindings ( );
        void InitRendererUI ( HWND hWnd , int w , int h );

    private:
        // Window/Core
        HWND   m_hWnd{};
        std::unique_ptr<Time>  m_Time;
        std::unique_ptr<Input> m_Input;
        std::unique_ptr<Scene> m_Scene;

        // Rendering
        std::unique_ptr<IRenderer>        m_Renderer;  // e.g., D3D11Renderer
        std::unique_ptr<D3D11SpriteBatch> m_Batch;     // sprites
        std::unique_ptr<D3D11DebugDraw>   m_Debug;     // lines/rects
        std::unique_ptr<DWriteTextHUD>    m_TextHUD;   // HUD text
        RenderSystem                      m_Render{};  // high-level facade

        // Misc
        bool m_comInitialized = false;   // CoInitializeEx succeeded?
        bool m_debugDrawEnabled = true;

        // Game
        std::unique_ptr<game::PlaySession> m_Session;
    };

} // namespace engine
