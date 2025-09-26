#include "gamePCH.h"
#include "DoorSystem.h"
#include "CEventMgr.h"
#include "EventDef.h"
#include "CDoor.h"
#include "CFadeEffect.h"

static bool  s_hasTarget = false;
static SCENE_TYPE s_targetScene = SCENE_TYPE::START;
static Vec2  s_targetPos = Vec2(0, 0);

size_t DoorSystem::s_subDoorEnter = 0;

void DoorSystem::Init() {
    s_subDoorEnter = CEventMgr::GetInst()->Subscribe(EVENT_TYPE::DOOR_ENTER, &DoorSystem::OnDoorEnter, 10);
}
void DoorSystem::Shutdown() {
    if (s_subDoorEnter) CEventMgr::GetInst()->Unsubscribe(EVENT_TYPE::DOOR_ENTER, s_subDoorEnter), s_subDoorEnter = 0;
}

void DoorSystem::SetNextDoorTarget(SCENE_TYPE sc, const Vec2& pos) {
    s_hasTarget = true; s_targetScene = sc; s_targetPos = pos;
}
bool DoorSystem::GetAndClearNextDoorTarget(SCENE_TYPE& outScene, Vec2& outPos) {
    if (!s_hasTarget) return false;
    outScene = s_targetScene; outPos = s_targetPos; s_hasTarget = false; return true;
}

void DoorSystem::OnDoorEnter(const tEvent& e) {
    CDoor* door = reinterpret_cast<CDoor*>(e.wParam);
    if (!door) return;

    SetNextDoorTarget(door->GetTargetScene(), door->GetTargetPosition());
    // 페이드 아웃 → 이후 GameFlowSystem의 FADE_COMPLETE(3)에서 씬 전환
    CFadeEffect::GetInst()->StartFadeOut(FADE_COLOR::WHITE, 0.5f, (uintptr_t)3);
}
