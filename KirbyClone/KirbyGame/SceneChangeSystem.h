#pragma once
#include "EventDef.h"

class CKirby;
class CScene;

class SceneChangeSystem {
public:
    static void Init();
    static void Shutdown();

    // 리스폰용: 다음 씬 전환에서 1회 적용될 스폰 좌표 지정
    static void SetNextSpawnOverride(const Vec2& pos);

private:
    static size_t s_subSceneChange;
    static CKirby* s_carryKirby;

    // 오버라이드 저장
    static bool s_hasSpawnOverride;
    static Vec2 s_spawnOverride;

    static void OnSceneChange(const tEvent& e);
    static CKirby* FindKirbyInScene(class CScene* sc);
};

