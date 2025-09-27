#include "gamePCH.h"
#include "BossSystem.h"
#include "CEventMgr.h"

#include "CSceneMgr.h"
#include "CScene.h"
#include "CBoss.h"
#include "CBossTrigger.h"
#include "CCamera.h"
#include "CUIGameHUD.h"

size_t BossSystem::s_subBossStart = 0;
size_t BossSystem::s_subStageClear = 0;

static void DeactivateAllBossTriggers(CScene* sc) {
    if (!sc) return;
    // 변경: TRIGGER 그룹에서 보스 트리거 찾아 비활성화
    const auto& trigs = sc->GetGroupObject(GROUP_TYPE::TRIGGER);
    for (auto* o : trigs) {
        if (!o || o->GetType() != OBJECT_TYPE::TRIGGER_BOSS) continue;
        if (auto* t = dynamic_cast<CBossTrigger*>(o)) t->SetActive(false);
    }
}

void BossSystem::Init() {
    auto& bus = *CEventMgr::GetInst();
    s_subBossStart = bus.Subscribe(EVENT_TYPE::BOSS_BATTLE_START, &BossSystem::OnBossBattleStart, 10);
    s_subStageClear = bus.Subscribe(EVENT_TYPE::STAGE_CLEAR, &BossSystem::OnStageClear, 10);
}
void BossSystem::Shutdown() {
    auto& bus = *CEventMgr::GetInst();
    if (s_subBossStart) bus.Unsubscribe(EVENT_TYPE::BOSS_BATTLE_START, s_subBossStart), s_subBossStart = 0;
    if (s_subStageClear)bus.Unsubscribe(EVENT_TYPE::STAGE_CLEAR, s_subStageClear), s_subStageClear = 0;
}

void BossSystem::OnBossBattleStart(const tEvent& e) {
    // 변경: 트리거 타입 치환
    CBossTrigger* trig = reinterpret_cast<CBossTrigger*>(e.lParam);
    Vec2 camLock = trig ? trig->GetLockPos() : Vec2(400.f, 300.f);

    auto* sc = CSceneMgr::GetInst()->GetCurScene(); if (!sc) return;

    CBoss* boss = nullptr;
    const auto& mons = sc->GetGroupObject(GROUP_TYPE::MONSTER);
    for (auto* o : mons) {
        if (auto* b = dynamic_cast<CBoss*>(o)) { boss = b; break; }
    }
    if (!boss) return;

    // 변경: 카메라 보스 모드 제거 → 즉시 위치 이동만
    CCamera::GetInst()->SetLookAtImmediate(camLock);
    // TODO(cam): 보스전 동안 카메라 따라가기/락/쉐이크는 카메라 리그로 이전

    // HUD: 보스 HP fill 애니 시작 (기존 유지)
    const auto& uiObjs = sc->GetGroupObject(GROUP_TYPE::UI);
    for (auto* o : uiObjs) if (auto* hud = dynamic_cast<CUIGameHUD*>(o)) { hud->StartBossHPFillAnimation(); break; }

    boss->StartBossEvent();
    DeactivateAllBossTriggers(sc); // ★ 변경 함수 사용
}

void BossSystem::OnStageClear(const tEvent& e) {
    (void)e;
    // TODO(flow): 스테이지 클리어 연출/카메라 해제는 카메라 리그/플로우 시스템으로 이동
}
