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

// === camera / anim / texture ===
#include "engine/Camera.h"
#include "engine/Anim.h"
#include "engine/Texture.h"

// === world ===
#include "engine/WorldSystem.h"

// === renderer / utils ===
#include "engine/IRenderer.h"
#include "engine/RenderSystem.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/D3D11SpriteBatch.h"

// === game objects ===
#include "game/Player.h"
#include "game/PlayerFSM.h"
#include "game/MonsterFactory.h"
#include "game/ProjectileSystem.h"
#include "game/HitVolumeSystem.h"
#include "game/HitVolumeFactory.h"

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
        void FixedUpdate ( double fixedDt ); // phisics / collider / jump ochestration
        void RenderFrame ( );               // tile / player / HUD / debug render

        // ---- Init/teardown helpers ----
        void InitBindings ( );
        void InitPlayerAndCamera ( const RECT & rcClient );
        void InitRendererUI ( HWND hWnd , int w , int h );
        void RegisterDefaultFactories ( );
        void InitSystems ( );
        bool LocateOwner ( int ownerId , engine::Vec2 & outPos , int& outFacing ); // for HitVolumeSystem
        
        // ---- Update helpers ----
        void StepPlayerFSM ( double fixedDt );
        void HandlePlayerEvents ( const std::vector<game::PlayerEvent>&evs );
        void UpdateMonsters ( double fixedDt );
        void CheckContactDamage ( );
        void BuildTargets ( std::vector<game::ProjectileSystem::Target>&projT ,
                            std::vector<game::HitVolumeSystem::Target>&hvT );
        void ApplyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>&phits );
        void ApplyHitVolumeHits ( const std::vector<game::HitVolumeSystem::HitEvent>&hvHits );
        bool LoadStage ( const char* jsonPath );
        
        // ---- Render helpers ----
        void RenderWorldBatch ( int ox , int oy , int sw , int sh );
        void RenderParallaxBG ( int ox , int oy , int sw , int sh );
        void RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh );
        void RenderHUD ( );

        // ---- Door Systems ----
        void CheckDoorInteract ( );

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
        Tex2D                               m_PlayerTex{};   // 플레이어 텍스처
        Tex2D                               m_EnemiesTex{};
        RenderSystem m_Render{};

        // --- Background (Parallax) ---
        Tex2D  m_BgTex{};

        // --- Camera/Player/Monster/Projectile/HitVolume ---
        Camera     m_Cam{};
        game::Player* m_Player{ nullptr };
        game::PlayerFSM m_PlayerFSM;
        game::PlayerFSM::Cfg m_playerFsmCfg{};
        std::vector<std::unique_ptr<game::Monster>> m_Monsters;
        game::ProjectileSystem m_projSys;
        game::HitVolumeSystem m_hitSys;

        // --- World ---
        WorldSystem m_World{};

        // --- ect ---
        bool m_comInitialized = false;  // CoInitializeEx 성공 여부
        bool m_debugDrawEnabled = true;

        // -- Stage & Reload & Door ---
        std::string m_stageJsonPath{ "assets/stages/stage01/stage.json" };
        double      m_reloadCooldown = 0.0;
        struct Door { RECT aabb{}; std::string target; };
        std::vector<Door> m_Doors;
    };

} // namespace engine
