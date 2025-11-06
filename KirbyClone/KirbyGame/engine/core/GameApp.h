#pragma once
//
// Responsibility: App bootstrap + main loop. Owns input/time/scene and wires renderer & session.
// Non-Goals:      Direct gameplay logic or low-level D3D state; those live in subsystems.
// Call-Context:   Win32 windowed app (single-threaded game loop).
//

#include <memory>

// Win32 HWND forward decl to keep header light
struct HWND__;
using HWND = HWND__*;

// Engine core
#include "engine/core/RenderSystem.h"   // value member → 필요 헤더

namespace engine {

    // fwd (pointers only in this header)
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
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );
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
        class Time   m_Time {};
        class Input  m_Input {};
        class Scene  m_Scene {};

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
