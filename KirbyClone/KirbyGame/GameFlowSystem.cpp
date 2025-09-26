#include "gamePCH.h"
#include "GameFlowSystem.h"
#include "CEventMgr.h"

#include "CSceneMgr.h"
#include "CScene.h"
#include "CFadeEffect.h"
#include "CSoundMgr.h"
#include "SceneChangeSystem.h"
#include "CKirby.h"

size_t GameFlowSystem::s_subGameOver = 0;
size_t GameFlowSystem::s_subFadeComplete = 0;

void GameFlowSystem::Init() {
    auto& bus = *CEventMgr::GetInst();
    s_subGameOver = bus.Subscribe(EVENT_TYPE::GAME_OVER, &GameFlowSystem::OnGameOver, 10);
    s_subFadeComplete = bus.Subscribe(EVENT_TYPE::FADE_COMPLETE, &GameFlowSystem::OnFadeComplete, 10);
}
void GameFlowSystem::Shutdown() {
    auto& bus = *CEventMgr::GetInst();
    if (s_subGameOver)     bus.Unsubscribe(EVENT_TYPE::GAME_OVER, s_subGameOver), s_subGameOver = 0;
    if (s_subFadeComplete) bus.Unsubscribe(EVENT_TYPE::FADE_COMPLETE, s_subFadeComplete), s_subFadeComplete = 0;
}

void GameFlowSystem::OnGameOver(const tEvent& e)
{
    auto* kirby = reinterpret_cast<CKirby*>(e.wParam);
    if (auto* sc = CSceneMgr::GetInst()->GetCurScene()) sc->SetPaused(true);
    CSoundMgr::GetInst()->PlaySFX(L"gameover");

    if (kirby) {
        kirby->DecLife(1);

        // 라이프가 남아있으면 현재 스테이지 리스폰
        if (kirby->GetLives() >= 0) {
            // 리스폰 시점에 HP/무적 초기화를 하도록 미리 준비
            kirby->ResetForRespawn(/*briefInvincible=*/true);

            // 리스폰 스폰 좌표 지정(간단 기본값; 필요하면 씬/체크포인트에서 받아오도록 확장)
            SceneChangeSystem::SetNextSpawnOverride(Vec2(256.f, 384.f));

            CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 0.5f, (uintptr_t)2); // code 2: respawn
        }
        else {
            // 완전 게임오버 → 타이틀
            CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 1.0f, (uintptr_t)1); // code 1: title
        }
    }
}

void GameFlowSystem::OnFadeComplete(const tEvent& e)
{
    const uintptr_t code = e.wParam;
    if (code == 1) {
        // 타이틀로
        CEventMgr::GetInst()->AddEvent({ EVENT_TYPE::SCENE_CHANGE, 0, (uintptr_t)SCENE_TYPE::START });
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
        if (auto* sc = CSceneMgr::GetInst()->GetCurScene()) sc->SetPaused(false);
    }
    else if (code == 2) {
        // 현재 스테이지 리스폰(같은 타입으로 재전환)
        const auto cur = CSceneMgr::GetInst()->GetCurSceneType();
        CEventMgr::GetInst()->AddEvent({ EVENT_TYPE::SCENE_CHANGE, 0, (uintptr_t)cur });
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
        if (auto* sc = CSceneMgr::GetInst()->GetCurScene()) sc->SetPaused(false);
    }
    else if (code == 3) {
        // 문 이동은 DoorSystem/SceneChangeSystem에서 위치 지정이 처리됨
        // 여기서는 씬만 바꿔주고 페이드 인
        // (문 이벤트 처리부에서 이미 SCENE_TYPE을 정해서 보냄)
        // → Boss/문 시스템 흐름대로 두기
    }
    else if (code == 4) {
        // 승리 후 타이틀
        CEventMgr::GetInst()->AddEvent({ EVENT_TYPE::SCENE_CHANGE, 0, (uintptr_t)SCENE_TYPE::START });
        CFadeEffect::GetInst()->StartFadeIn(FADE_COLOR::WHITE, 0.5f, 0);
    }
}
