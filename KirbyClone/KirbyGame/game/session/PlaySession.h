#pragma once
//
// Responsibility: Run-time game session that orchestrates world/camera, player FSM, combat, monsters, and debug/HUD rendering.
// Non-Goals    : Asset loading policy, renderer creation, or editor/runtime switching.
// Call-Context : Owned by GameApp; initialized once per stage, ticked on fixed update and rendered between Begin/EndFrame.
//

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// === Forward decls to keep header light ===
namespace engine {
    class IRenderer;
    class RenderSystem;
    class DWriteTextHUD;
    class Scene;
    class Input;
    struct Tex2D;
}

namespace game {
    class Player;
    class Monster;
    struct PlayerEvent;
}

// === Required full defs (by-value members) ===
#include "engine/world/WorldSystem.h"
#include "engine/world/Camera.h"
#include "engine/render/Texture.h"
#include "engine/util/Types.h"                  // engine::IntRect
#include "game/entities/player/PlayerFSM.h"     // PlayerFSM is a by-value member
#include "game/projectile/ProjectileSystem.h"   // by-value member
#include "game/combat/HitVolumeSystem.h"        // by-value member
#include "game/entities/monsters/Monster.h"
#include "game/entities/monsters/MonsterTypes.h"
#include "game/data/StageCSV.h"                 // DoorCSV
#include "game/session/SessionState.h"          // SessionState
#include "game/effects/Fade2D.h"
#include "game/render/ZOrder.h"
#include "game/world/TileLayerRuntime.h"

namespace game {

    class PlaySession {
    public:
        struct CreateDesc {
            engine::IRenderer*          renderer = nullptr;     // for size queries
            engine::RenderSystem*       renderSys = nullptr;
            engine::DWriteTextHUD*      textHUD = nullptr;
            engine::Scene*              scene = nullptr;        // to spawn Player
            engine::IntRect             rcClient{};             // initial client rect
            game::SessionState*         session = nullptr;
        };

        ~PlaySession ( );

        // 1) Lifetime / init
        void Initialize ( const CreateDesc& d );
        void OnResize ( int sw , int sh );

        // 2) Stage load (world/bg/player start/camera bounds)
        bool LoadStage ( const char* jsonPath /*stage.json path*/ );
        bool ReloadStage ( ) { return LoadStage ( m_stageJsonPath.c_str ( ) ); }

        // 3) Fixed update (FSM/combat/monsters/camera)
        void FixedUpdate ( double fixedDt , const engine::Input& input );

        // 4) Render (called between GameApp Begin/EndFrame)
        void RenderParallaxBG ( int ox , int oy , int sw , int sh );
        void RenderWorld ( int ox , int oy , int sw , int sh );
        void RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh , bool drawEnabled );
        void RenderHUD ( int fps , double fixedDt );  // 이름 변경 예정
        void RenderOverlayFade ( int sw , int sh ); // (later) fade may move out
        void RenderGameHUDSprites ( int sw , int sh );
        void RenderGameOverOverlay ( int sw , int sh );

        // 5) Camera/world helpers
        std::pair<int , int> CameraOffsetInt ( ) const { return m_Cam.OffsetInt ( ); }
        engine::Camera& Camera ( )                     { return m_Cam; }
        const engine::WorldSystem& World ( )     const { return m_World; }
        engine::WorldSystem& World ( )                 { return m_World; }
        engine::IntRect WorldRectPx ( )          const { return m_World.WorldRectPx ( ); }
        int PlayerFacing ( )                     const { return m_PlayerFSM.Facing ( ); }

        // (temp) player handle
        game::Player* Player ( ) const { return m_Player; }

        // (temp) expose player events to GameApp if needed
        void DrainPlayerEvents ( std::vector<game::PlayerEvent>& out );

        // --- Fade API (z-layer selectable) ---
        void StartFadeIn ( float seconds , uint32_t rgb = 0xFFFFFFu , int16_t z = game::Z::OverlayTop );
        void StartFadeOut ( float seconds , uint32_t rgb = 0xFFFFFFu , int16_t z = game::Z::OverlayTop );
        bool IsFading ( ) const { return m_fade.Active ( ); }

        // ---- Door / Transition API ----
        void StartTransitionTo ( const std::string& target ,
                                 float fadeOutSec = 0.25f ,
                                 float fadeInSec = 0.20f ,
                                 const char* spawnOverride = nullptr );

        // ---- Clear Flow (public trigger/query) ----
        void BeginClearSequence ( );
        bool IsClearSequenceActive ( ) const;

        // Player slots
        enum class PlayerSlot : uint8_t { P1 = 0 , P2 = 1 , Count };

        // === Life API ===
        void ResetLives ( int initialLives = 2 );   // 새 런(허브 시작) 전용
        int  Lives ( ) const noexcept;
        bool IsGameOver ( ) const noexcept;

    private:
        // Internals
        void initPlayerAndCamera ( const engine::IntRect& rcClient );
        void updateCameraBoundsForWorld ( int sw , int sh );
        void registerDefaultFactories ( );
        void initCombatSystems ( );
        void updateMonsters ( double fixedDt , const engine::Input& input );
        void updateItems ( double fixedDt );
        void handlePlayerEvents ( const std::vector<game::PlayerEvent>& evs );
        void buildTargets ( std::vector<game::ProjectileSystem::Target>& projT ,
                                             std::vector<game::HitVolumeSystem::Target>& hvT );
        void applyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>& phits );
        void applyHitVolumeHits ( const std::vector<game::HitVolumeSystem::HitEvent>& hvHits );
        void handleHitVolumeDespawns ( const std::vector<game::HitVolumeSystem::DespawnEvent>& devs );
        void flushPendingSpawns ( );
        bool checkDoorInteract ( ); // Player AABB vs Door AABB overlap
        void updateTransition ( double fixedDt );

        // --- Clear Flow ---
        enum class ClearState { Idle , Emblem , AutoPilot , Dance , FadeOut , SaveAndHub , FadeIn };

        // Debug helper: convert ClearState to wide string (for logs / HUD)
        static const wchar_t* ClearStateName ( ClearState st );

        struct ClearCtx {
            ClearState st{ ClearState::Idle };
            float t{ 0.f };
            // durations
            float tEmblem{ 1.0f };
            float tAuto{ 1.0f };
            float tDance{ 2.5f };
            float tFade{ 0.6f };
            // ids
            std::string stageId;
            std::string hubSpawnKey;
        } m_clear;

        void updateClearFlow ( double dt );

        // --- Boss Arena / Camera Lock ---
        void updateBossCameraLock ( );
        bool isBossAlive ( ) const;

        struct LifeState {
            // 설정
            int   initialLives = 2;
            bool  useSharedLives = true;   // true: 공용 목숨, false: per-player 목숨 (나중 확장용)

            // 공용 목숨 모드
            int   sharedLives = 2;

            // per-player 모드 대비 (지금은 안 씀)
            int   perPlayerLives[ static_cast< int >( PlayerSlot::Count ) ]{};

            // 게임오버 진행 상태
            bool  gameOver = false;            
            bool  gameOverScreenActive = false;
            float gameOverScreenT = 0.f;       
            float gameOverScreenMin = 2.f;     
        } m_life{};

        // 죽음 처리 진입점 (PlayerEvent::Died 처리에서 호출)
        void onPlayerDied ( PlayerSlot who );

    private:
        // Provided handles (non-owning)
        engine::IRenderer*          m_Renderer = nullptr;
        engine::RenderSystem*       m_RenderSys = nullptr;
        engine::DWriteTextHUD*      m_TextHUD = nullptr;
        engine::Scene*              m_Scene = nullptr;

        // SessionState
        game::SessionState*         m_Session = nullptr;

        // Session-owned runtime
        engine::WorldSystem         m_World;
        engine::Camera              m_Cam;

        // Stage tile layers
        std::vector<game::TileLayerRuntime> m_TileLayers;

        game::Player*               m_Player = nullptr;
        game::PlayerFSM             m_PlayerFSM;
        game::PlayerFSM::Cfg        m_playerFsmCfg{ .jumpSpeed = 700.f, .coyoteMs = 0.08f, .bufferMs = 0.10f, .dropMs = 0.20f };

        // Resources
        engine::Tex2D m_PlayerTex{};
        engine::Tex2D m_EnemiesTex{};
        engine::Tex2D m_WhiteTex{};
        engine::Tex2D m_BgTex{};
        engine::Tex2D m_HudTex{};
        engine::Tex2D m_GameOverTex{};

        // Options
        std::string m_stageJsonPath{ "assets/stages/stage01/stage.json" };
        std::string m_stageId{};

        // --- Items / pickups (clear emblem etc.) ---
        struct ItemRuntime {
            enum class Kind { ClearEmblem };

            Kind kind = Kind::ClearEmblem;

            // Axis-aligned bounds in world pixel space (left/top/width/height)
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;

            bool collected = false;
        };

        std::vector<ItemRuntime> m_Items;

        // ---- Combat / monsters ----
        std::vector<std::unique_ptr<game::Monster>> m_Monsters;
        std::vector<game::SpawnSpec>                m_pendingMonsterSpawns;
        game::ProjectileSystem                      m_projSys;
        game::HitVolumeSystem                       m_hitSys;
        std::vector<game::PlayerEvent>              m_pendingPlayerEvents;
        std::unordered_map<std::string , int>        m_playerHVActive;

        // --- Fade Effect ---
        game::Fade2D m_fade;

        // ---- Door / Transition ----
        std::vector<game::DoorCSV> m_Doors; // StageCSV format as-is
        struct Transition {
            enum State { Idle , FadingOut , Loading , FadingIn } state = Idle;
            std::string target;
            float       fadeOut = 0.25f;
            float       fadeIn = 0.20f;
            std::string spawn;
        } m_trans;

        // --- Boss Arena / Camera Lock ---
        bool             m_hasBossArena = false;
        engine::IntRect  m_bossArena{ 0,0,0,0 };
        bool             m_bossCamLocked = false;
        engine::IntRect  m_worldRectFull{ 0,0,0,0 }; // full-world cached bounds (restore on unlock)

        // --- Camera rect blend (for smooth lock/unlock) ---
        struct CamRectBlend {
            bool            active = false;
            float           t = 0.f;
            float           dur = 0.6f;           // seconds
            engine::IntRect from{ 0,0,0,0 };
            engine::IntRect to{ 0,0,0,0 };
        } m_camBlend;

        void applyCamRectBlend ( float dt );
        static engine::IntRect LerpRect ( const engine::IntRect& a ,
                                          const engine::IntRect& b ,
                                          float t );
    };

} // namespace game
