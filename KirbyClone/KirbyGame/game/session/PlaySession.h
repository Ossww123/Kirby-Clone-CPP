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
        void RenderHUD ( int fps , double fixedDt );
        void RenderOverlayFade ( int sw , int sh ); // (later) fade may move out

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

        // --- Fade API ---
        void StartFadeIn ( float seconds , uint32_t rgb = 0xFFFFFFu );
        void StartFadeOut ( float seconds , uint32_t rgb = 0xFFFFFFu );
        bool IsFading ( ) const { return m_fade.mode != Fade::None; }

        // ---- Door / Transition API ----
        void StartTransitionTo ( const std::string& target ,
                                 float fadeOutSec = 0.25f ,
                                 float fadeInSec = 0.20f ,
                                 const char* spawnOverride = nullptr );

        // ---- Clear Flow (public trigger/query) ----
        void BeginClearSequence ( );
        bool IsClearSequenceActive ( ) const;

    private:
        // Internals
        void initPlayerAndCamera ( const engine::IntRect& rcClient );
        void updateCameraBoundsForWorld ( int sw , int sh );
        void registerDefaultFactories ( );
        void initCombatSystems ( );
        void updateMonsters ( double fixedDt , const engine::Input& input );
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

        game::Player*               m_Player = nullptr;
        game::PlayerFSM             m_PlayerFSM;
        game::PlayerFSM::Cfg        m_playerFsmCfg{ .jumpSpeed = 700.f, .coyoteMs = 0.08f, .bufferMs = 0.10f, .dropMs = 0.20f };

        // Resources
        engine::Tex2D m_PlayerTex{};
        engine::Tex2D m_EnemiesTex{};
        engine::Tex2D m_WhiteTex{};
        engine::Tex2D m_BgTex{};

        // Options
        std::string m_stageJsonPath{ "assets/stages/stage01/stage.json" };
        std::string m_stageId{};

        // ---- Combat / monsters ----
        std::vector<std::unique_ptr<game::Monster>> m_Monsters;
        std::vector<game::SpawnSpec>                m_pendingMonsterSpawns;
        game::ProjectileSystem                      m_projSys;
        game::HitVolumeSystem                       m_hitSys;
        std::vector<game::PlayerEvent>              m_pendingPlayerEvents;
        std::unordered_map<std::string , int>        m_playerHVActive;

        // --- Fade Effect ---
        struct Fade {
            enum Mode { None , In , Out } mode = None;
            float    t = 0.f;           // elapsed
            float    dur = 0.f;           // total duration
            uint32_t rgb = 0xFFFFFFu;     // color (white default)
        } m_fade;

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
