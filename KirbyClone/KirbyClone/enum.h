#pragma once

enum class KEY_STATE
{
    NONE,   // 이전에도 안눌림, 지금도 안눌림
    TAP,    // 이전에 안눌림, 지금 눌림
    HOLD,   // 이전에도 눌림, 지금도 눌림
    AWAY,   // 이전에 눌림, 지금 안눌림
};

enum class KEY
{
    LEFT,
    RIGHT,
    UP,
    DOWN,

    Q, W, E, R, T, Y,
    A, S, D, F, G, H,
    Z, X, C, V, B,

    // 숫자 키 추가
    ALPHA_1, ALPHA_2, ALPHA_3, ALPHA_4,
    ALPHA_5, ALPHA_6, ALPHA_7, ALPHA_8,
    ALPHA_9, ALPHA_0,

    SPACE,
    ENTER,
    ESC,

    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,

    LAST,  // enum의 끝
};

enum class GROUP_TYPE
{
    DEFAULT,
    PLAYER,
    MONSTER,
    PROJ_PLAYER,    // 플레이어 투사체
    PROJ_MONSTER,   // 몬스터 투사체

    END = 32,
};

enum class SCENE_TYPE
{
    TOOL,
    START,
    // STAGE_01,
    // STAGE_02,

    END,
};

// 이벤트 타입 추가
enum class EVENT_TYPE
{
    CREATE_OBJECT,      // 오브젝트 생성 (wParam: GROUP_TYPE, lParam: CObject*)
    DELETE_OBJECT,      // 오브젝트 삭제 (lParam: CObject*)
    SCENE_CHANGE,       // 씬 변경 (lParam: SCENE_TYPE)
    COLLISION_ENTER,    // 충돌 시작 이벤트
    COLLISION_EXIT,     // 충돌 종료 이벤트

    END
};

// 플레이어 상태 열거형
enum class PLAYER_STATE
{
    IDLE,
    WALK,
    JUMP,
    END
};

// 에디터 모드 열거형
enum class EDITOR_MODE
{
    NONE,           // 기본 모드
    PLACE_MONSTER,  // 몬스터 배치 모드
    SELECT,         // 선택 모드
    ERASE,          // 삭제 모드
    CAMERA_MOVE,    // 카메라 이동 모드 (추후 구현)
    END
};
