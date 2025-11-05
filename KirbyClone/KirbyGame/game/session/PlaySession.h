#pragma once
#include <memory>
#include <vector>
#include <string>
#include <windows.h>

#include "engine/IRenderer.h"
#include "engine/D3D11SpriteBatch.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/WorldSystem.h"
#include "engine/Camera.h"
#include "engine/Scene.h"
#include "engine/Texture.h"
#include "engine/Collision.h"

#include "game/Player.h"
#include "game/PlayerFSM.h"
#include "game/StageDesc.h"
#include "game/StageCSV.h"

// Combat / Monster
#include "game/ProjectileSystem.h"
#include "game/ProjectileFactory.h"
#include "game/HitVolumeSystem.h"
#include "game/HitVolumeFactory.h"
#include "game/Monster.h"
#include "game/MonsterFactory.h"

namespace game {

    class PlaySession {
    public:
        struct CreateDesc {
            engine::IRenderer* renderer = nullptr;   // for size queries
            engine::D3D11SpriteBatch* batch = nullptr;
            engine::D3D11DebugDraw* debug = nullptr;
            engine::DWriteTextHUD* textHUD = nullptr;
            engine::Scene* scene = nullptr;   // to spawn Player
            RECT                      rcClient{};           // initial client rect
        };

        ~PlaySession ( );

        // 1) 수명/초기화
        void Initialize ( const CreateDesc& d );
        void OnResize ( int sw , int sh );

        // 2) 스테이지 로드(월드/배경/플레이어 위치/카메라 경계)
        bool LoadStage ( const char* jsonPath );
        bool ReloadStage ( ) { return LoadStage ( m_stageJsonPath.c_str ( ) ); }

        // 3) (후속 단계에서 확장) 고정 업데이트/전투/도어/페이드
        void FixedUpdate ( double fixedDt , const engine::Input& input ); // FSM/전투/몬스터/카메라

        // 4) 렌더 (GameApp Begin/EndFrame 사이에서 호출)
        void RenderParallaxBG ( int ox , int oy , int sw , int sh );
        void RenderWorld ( int ox , int oy , int sw , int sh );
        void RenderDebugGridAndColliders ( int ox , int oy , int sw , int sh , bool drawEnabled );
        void RenderHUD ( int fps , double fixedDt );
        void RenderOverlayFade ( int sw , int sh ); // (후속 단계) 페이드 이동 예정

        // 5) 카메라/월드 헬퍼
        std::pair<int , int>    CameraOffsetInt ( ) const { return m_Cam.OffsetInt ( ); }
        engine::Camera&         Camera ( )                { return m_Cam; }
        const engine::WorldSystem&  World ( )       const { return m_World; }
        engine::WorldSystem&        World ( )             { return m_World; }
        RECT WorldRectPx ( )                        const { return m_World.WorldRectPx ( ); }
        int PlayerFacing ( )                        const { return m_PlayerFSM.Facing ( ); }

        // (임시) 플레이어 핸들
        game::Player* Player ( ) const { return m_Player; }

        // (임시) GameApp이 필요하면 플레이어 이벤트를 외부로 전달할 수도 있음
        void DrainPlayerEvents ( std::vector<game::PlayerEvent>&out );

        // --- Fade API ---
        void StartFadeIn ( float seconds , uint32_t rgb = 0xFFFFFFu );
        void StartFadeOut ( float seconds , uint32_t rgb = 0xFFFFFFu );
        bool IsFading ( ) const { return m_fade.mode != Fade::None; }
        
        // ---- Door / Transition API ----
        void StartTransitionTo ( const std::string & target , float fadeOutSec = 0.25f , float fadeInSec = 0.20f );


    private:
        // 내부 유틸
        void initPlayerAndCamera ( const RECT& rcClient );
        void updateCameraBoundsForWorld ( int sw , int sh );
        void registerDefaultFactories ( );
        void initCombatSystems ( );
        void updateMonsters ( double fixedDt , const engine::Input& input );
        void handlePlayerEvents ( const std::vector<game::PlayerEvent>& evs );
        void buildTargets ( std::vector<game::ProjectileSystem::Target>& projT ,
                          std::vector<game::HitVolumeSystem::Target>& hvT );
        void applyProjectileHits ( const std::vector<game::ProjectileSystem::HitEvent>& phits );
        void applyHitVolumeHits ( const std::vector<game::HitVolumeSystem::HitEvent>& hvHits );
        void flushPendingSpawns ( );
        bool checkDoorInteract ( ); // Player AABB와 문 AABB 오버랩 감지
        void updateTransition ( double fixedDt );

        // --- Boss Arena / Camera Lock ---
        void updateBossCameraLock ( );
        bool isBossAlive ( ) const;

    private:
        // 외부 제공 핸들(비소유)
        engine::IRenderer* m_Renderer = nullptr;
        engine::D3D11SpriteBatch* m_Batch = nullptr;
        engine::D3D11DebugDraw* m_Debug = nullptr;
        engine::DWriteTextHUD* m_TextHUD = nullptr;
        engine::Scene* m_Scene = nullptr;

        // 런타임 상태(세션 소유)
        engine::WorldSystem m_World;
        engine::Camera      m_Cam;

        game::Player* m_Player = nullptr;
        game::PlayerFSM     m_PlayerFSM;
        game::PlayerFSM::Cfg m_playerFsmCfg{ .jumpSpeed = 700.f, .coyoteMs = 0.08f, .bufferMs = 0.10f, .dropMs = 0.20f };

        // 리소스
        engine::Tex2D m_PlayerTex{};
        engine::Tex2D m_EnemiesTex{};
        engine::Tex2D m_WhiteTex{};
        engine::Tex2D m_BgTex{};

        // 옵션
        std::string m_stageJsonPath{ "assets/stages/stage01/stage.json" };

        // ---- 전투/몬스터 런타임 ----
        std::vector<std::unique_ptr<game::Monster>> m_Monsters;
        std::vector<game::SpawnSpec> m_pendingMonsterSpawns;
        game::ProjectileSystem m_projSys;
        game::HitVolumeSystem  m_hitSys;
        std::vector<game::PlayerEvent> m_pendingPlayerEvents;

        // --- Fade Effect ---
        struct Fade {
            enum Mode { None , In , Out } mode = None;
            float t = 0.f;       // 진행 시간
            float dur = 0.f;     // 총 길이
            uint32_t rgb = 0xFFFFFFu; // 색상(밝은 흰색 기본)
        } m_fade;

        // ---- Door / Transition ----
        std::vector<game::DoorCSV> m_Doors; // StageCSV 포맷 그대로 사용
        struct Transition {
            enum State { Idle , FadingOut , Loading , FadingIn } state = Idle;
            std::string target;
            float fadeOut = 0.25f;
            float fadeIn = 0.20f;
        } m_trans;

        // --- Boss Arena / Camera Lock ---
        bool m_hasBossArena = false;
        RECT m_bossArena{ 0,0,0,0 };
        bool m_bossCamLocked = false;
        RECT m_worldRectFull{ 0,0,0,0 }; // 풀월드 경계 캐시(해제 시 복원)

        // --- Camera rect blend (for smooth lock/unlock) ---
        struct CamRectBlend {
            bool  active = false;
            float t = 0.f;       // 진행 시간
            float dur = 0.6f;    // 보간 지속(초) - 취향껏 조절
            RECT  from{ 0,0,0,0 };
            RECT  to{ 0,0,0,0 };
        } m_camBlend;
        void applyCamRectBlend ( float dt );
        static RECT LerpRect ( const RECT & a , const RECT & b , float t );
    };

} // namespace game
