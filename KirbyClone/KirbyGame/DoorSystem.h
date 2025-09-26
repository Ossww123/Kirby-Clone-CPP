#pragma once

#include "EventDef.h"

class DoorSystem {
public:
    static void Init();
    static void Shutdown();

    // 다음 씬 전환에 사용할 목표 정보(FADE_COMPLETE(3)에서 조회)
    static void SetNextDoorTarget(SCENE_TYPE scene, const Vec2& pos);
    static bool GetAndClearNextDoorTarget(SCENE_TYPE& outScene, Vec2& outPos);

private:
    static size_t s_subDoorEnter;
    static void OnDoorEnter(const tEvent& e); // w: CDoor*, l: SCENE_TYPE
};
