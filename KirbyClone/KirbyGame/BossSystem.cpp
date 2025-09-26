#include "gamePCH.h"
#include "BossSystem.h"
#include "CEventMgr.h"

#include "CSceneMgr.h"
#include "CScene.h"
#include "CBoss.h"
#include "CTile.h"
#include "CCamera.h"
#include "CUIGameHUD.h"

size_t BossSystem::s_subBossStart = 0;
size_t BossSystem::s_subStageClear = 0;

static void RemoveAllBossTriggerBoxes(CScene* sc) {
    if (!sc) return;
    const auto& tiles = sc->GetGroupObject(GROUP_TYPE::TILE);
    for (auto* o : tiles) {
        if (!o || o->GetType() != OBJECT_TYPE::TILE_TRIGGER) continue;
        if (auto* t = dynamic_cast<CTile*>(o)) {
            if (t->GetVisualType() == TILE_VISUAL_TYPE::BOSS_TRIGGER) t->SetTriggerActive(false);
        }
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
    CTile* trig = reinterpret_cast<CTile*>(e.lParam);
    Vec2 camLock = trig ? trig->GetBossLockPosition() : Vec2(400.f, 300.f);

    auto* sc = CSceneMgr::GetInst()->GetCurScene(); if (!sc) return;

    CBoss* boss = nullptr;
    const auto& mons = sc->GetGroupObject(GROUP_TYPE::MONSTER);
    for (auto* o : mons) {
        if (auto* b = dynamic_cast<CBoss*>(o)) { boss = b; break; }
    }
    if (!boss) return;

    CCamera::GetInst()->StartBossMode(camLock);

    // HUD: 보스 HP fill 애니 시작
    const auto& uiObjs = sc->GetGroupObject(GROUP_TYPE::UI);
    for (auto* o : uiObjs) if (auto* hud = dynamic_cast<CUIGameHUD*>(o)) { hud->StartBossHPFillAnimation(); break; }

    boss->StartBossEvent();
    RemoveAllBossTriggerBoxes(sc);
}

void BossSystem::OnStageClear(const tEvent& e) {
    // e.lParam: CBoss*
    (void)e;
    // 여기서는 연출 트리거/사운드/카메라 등 공통만 처리하고,
    // 씬 전환은 GameFlowSystem 혹은 다른 이벤트로 이어가도 된다.
}
