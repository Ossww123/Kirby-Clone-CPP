#pragma once
#include <windows.h>
#include <memory>
#include <cwchar>
#include <vector>
#include <string>

// === engine / core ===
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"

// === anim / texture ===
#include "engine/Anim.h"
#include "engine/Texture.h"

// === renderer / utils ===
#include "engine/IRenderer.h"
#include "engine/RenderSystem.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/D3D11SpriteBatch.h"

#include "game/PlaySession.h"

namespace game { struct MonsterCSV; }

namespace engine {

    class GameApp {
    public:
        // 소멸자 정의는 .cpp에서 (unique_ptr default_delete가 완전형을 보게 하기 위함)
        ~GameApp ( );

        void Init ( HWND hWnd );
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );
        void OnResize ( int w , int h );
        bool DoOneFrame ( );

    private:
        void FixedUpdate ( double fixedDt ); // physics / collider / jump orchestration
        void RenderFrame ( );               // tile / player / HUD / debug render

        // ---- Init/teardown helpers ----
        void InitBindings ( );
        void InitRendererUI ( HWND hWnd , int w , int h );

    private:
        // --- Window/Core ---
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        // --- Rendering ---
        std::unique_ptr<IRenderer>          m_Renderer;  // D3D11Renderer
        std::unique_ptr<D3D11SpriteBatch>   m_Batch;     // 스프라이트 일괄 렌더
        std::unique_ptr<D3D11DebugDraw>     m_Debug;     // 라인/박스 디버그 드로우
        std::unique_ptr<DWriteTextHUD>      m_TextHUD;   // DirectWrite HUD
        RenderSystem m_Render{};

        // --- ect ---
        bool m_comInitialized = false;  // CoInitializeEx 성공 여부
        bool m_debugDrawEnabled = true;

        std::unique_ptr<game::PlaySession> m_Session;
    };
} // namespace engine
