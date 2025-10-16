#pragma once
#include <windows.h>
#include <memory>
#include <cwchar>
#include <vector>
#include <string>

// === 엔진 코어 ===
#include "engine/Time.h"
#include "engine/Input.h"
#include "engine/Scene.h"
#include "engine/Math.h"

// === 카메라/애니/텍스처 ===
#include "engine/Camera.h"
#include "engine/Anim.h"
#include "engine/Texture.h"

// === 월드 ===
#include "engine/WorldSystem.h"

// === 렌더러 & 유틸 ===
#include "engine/IRenderer.h"
#include "engine/RenderSystem.h"
#include "engine/D3D11Renderer.h"
#include "engine/D3D11DebugDraw.h"
#include "engine/DWriteText.h"
#include "engine/D3D11SpriteBatch.h"

// === 게임 오브젝트 ===
#include "game/Player.h"
#include "game/PlayerFSM.h"
#include "game/MonsterFactory.h"
#include "game/Projectile.h"

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
        bool LoadStageFromCSV ( const char* folder );
        bool ReloadStage ( );
        void SpawnMonsterFromRow ( const RECT& worldRect , const game::MonsterCSV& r );

    private:
        void InitBindings ( );
        void FixedUpdate ( double fixedDt ); // 물리/충돌/점프 오케스트레이션
        void RenderFrame ( );               // 타일/플레이어/HUD/디버그 렌더

    private:
        // --- 윈도우/코어 ---
        HWND   m_hWnd{};
        Time   m_Time{};
        Input  m_Input{};
        Scene  m_Scene{};

        // --- 렌더링 ---
        std::unique_ptr<IRenderer>          m_Renderer;  // D3D11Renderer
        std::unique_ptr<D3D11SpriteBatch>   m_Batch;     // 스프라이트 일괄 렌더
        std::unique_ptr<D3D11DebugDraw>     m_Debug;     // 라인/박스 디버그 드로우
        std::unique_ptr<DWriteTextHUD>      m_TextHUD;   // DirectWrite HUD
        Tex2D                               m_PlayerTex{};   // 플레이어 텍스처
        RenderSystem m_Render{};

        // --- 카메라/플레이어/몬스터/투사체 ---
        Camera     m_Cam{};
        game::Player* m_Player{ nullptr };
        game::PlayerFSM m_PlayerFSM;
        std::vector<std::unique_ptr<game::Monster>> m_Monsters;
        std::vector<std::unique_ptr<game::Projectile>> m_Projectiles;

        // --- 월드 ---
        WorldSystem m_World{};

        // --- 물리 상태(필요 시 사용) ---
        Vec2  m_Vel{ 0.f, 0.f };
        bool  m_Grounded = false;

        // --- 점프/물리 튜닝 타이머 ---
        float m_coyoteTimer = 0.f;
        float m_jumpBufferTimer = 0.f;
        float m_dropThroughTimer = 0.f;

        // --- 점프 파라미터(튜닝값) ---
        float m_jumpSpeed = 700.f;   // 초기 상승 속도
        float m_coyoteMs = 0.08f;   // 코요테
        float m_bufferMs = 0.10f;   // 버퍼
        float m_dropMs = 0.20f;   // ↓+점프 드롭 유지 시간

        // --- 기타 ---
        bool m_comInitialized = false;  // CoInitializeEx 성공 여부
        bool m_isMoving = false;
        bool m_debugDrawEnabled = true;
        int m_facing = 1;

        // -- 라로드 관련 ---
        std::string m_stageFolder{ "assets/stage01" };
        double      m_reloadCooldown = 0.0;
    };

} // namespace engine
