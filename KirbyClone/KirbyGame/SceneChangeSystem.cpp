#include "gamePCH.h"
#include "SceneChangeSystem.h"
#include "CEventMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "DoorSystem.h"
#include "CKirby.h"
#include "CRigidBody.h"

size_t SceneChangeSystem::s_subSceneChange = 0;
CKirby* SceneChangeSystem::s_carryKirby = nullptr;
bool SceneChangeSystem::s_hasSpawnOverride = false;
Vec2 SceneChangeSystem::s_spawnOverride = Vec2(0, 0);

static CKirby* FindKirbyFromGroup(const std::vector<CObject*>& v) {
    if (v.empty()) return nullptr;
    for (auto* o : v) if (auto* k = dynamic_cast<CKirby*>(o)) return k;
    return nullptr;
}

CKirby* SceneChangeSystem::FindKirbyInScene(CScene* sc)
{
    if (!sc) return nullptr;
    const auto& pv = sc->GetGroupObject(GROUP_TYPE::PLAYER);
    if (auto* k = FindKirbyFromGroup(pv)) return k;

    // 혹시 다른 그룹에 잘못 들어간 경우 전체 검색(안전장치)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i) {
        const auto& gv = sc->GetGroupObject((GROUP_TYPE)i);
        if (auto* k = FindKirbyFromGroup(gv)) return k;
    }
    return nullptr;
}

void SceneChangeSystem::Init()
{
    s_subSceneChange = CEventMgr::GetInst()->Subscribe(
        EVENT_TYPE::SCENE_CHANGE, &SceneChangeSystem::OnSceneChange, /*prio*/100);
}

void SceneChangeSystem::Shutdown()
{
    if (s_subSceneChange) {
        CEventMgr::GetInst()->Unsubscribe(EVENT_TYPE::SCENE_CHANGE, s_subSceneChange);
        s_subSceneChange = 0;
    }
}

void SceneChangeSystem::SetNextSpawnOverride(const Vec2& pos)
{
    s_hasSpawnOverride = true;
    s_spawnOverride = pos;
}

void SceneChangeSystem::OnSceneChange(const tEvent& e)
{
    const SCENE_TYPE next = static_cast<SCENE_TYPE>(e.lParam);

    // 1) Detach carry
    if (auto* cur = CSceneMgr::GetInst()->GetCurScene()) {
        if (auto* k = FindKirbyInScene(cur)) {
            if (cur->DetachObject(k)) s_carryKirby = k;
        }
    }

    // 2) 실제 전환
    CSceneMgr::GetInst()->ChangeScene(next);

    // 3) Attach + 스폰 적용
    if (auto* ns = CSceneMgr::GetInst()->GetCurScene()) {
        if (s_carryKirby) {
            ns->AddObject(s_carryKirby, GROUP_TYPE::PLAYER);

            // 우선순위: DoorSystem 스폰 > Respawn 오버라이드 > 유지
            SCENE_TYPE doorScene; Vec2 doorSpawn;
            if (DoorSystem::GetAndClearNextDoorTarget(doorScene, doorSpawn) && doorScene == next) {
                s_carryKirby->SetPos(doorSpawn);
                if (auto* rb = s_carryKirby->GetRigidBody()) rb->SetGround(false);
            }
            else if (s_hasSpawnOverride) {
                s_carryKirby->SetPos(s_spawnOverride);
                if (auto* rb = s_carryKirby->GetRigidBody()) rb->SetGround(false);
                s_hasSpawnOverride = false;
            }

            s_carryKirby = nullptr;
        }
    }
}
