#pragma once
#include <cstdint>

enum class EVENT_TYPE : uint32_t {
    // 씬/오브젝트 수명주기
    CREATE_OBJECT,
    DELETE_OBJECT,
    SCENE_CHANGE,

    // 전투/피격
    PLAYER_DAMAGE,             // w: CKirby*, l: Vec2* (new로 할당된 넉백 방향; 리스너가 delete)
    MONSTER_DAMAGE,            // w: CMonster*, l: CProjectile*
    PLAYER_SLIDE_KICK_RECOIL,  // w: CProjectile*, l: CObject* (맞은 대상), 필요 시만 사용

    // 게임 플로우
    PLAYER_DEATH,              // w: CKirby*, l: 0
    GAME_OVER,                 // w: CKirby*, l: 0
    FADE_COMPLETE,             // w: 콜백코드(1,2,3,4…), l: 0

    // 보스/문/트리거
    BOSS_BATTLE_START,         // w: 0, l: CTile*
    STAGE_CLEAR,               // w: 0, l: CBoss*
    DOOR_ENTER,                // w: CDoor*, l: SCENE_TYPE

    //
    COLLISION_ENTER,
    COLLISION_EXIT,

    END
};

struct tEvent {
    EVENT_TYPE eType;
    uintptr_t  wParam;
    uintptr_t  lParam;

    tEvent() : eType(EVENT_TYPE::END), wParam(0), lParam(0) {}
    tEvent(EVENT_TYPE t, uintptr_t w = 0, uintptr_t l = 0) : eType(t), wParam(w), lParam(l) {}
};
