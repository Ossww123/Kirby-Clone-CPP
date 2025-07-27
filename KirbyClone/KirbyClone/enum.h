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

    Q, W, E, R, T, Y, U, I, O, P,
    A, S, D, F, G, H, J, K, L,
    Z, X, C, V, B, N, M,

    // 숫자 키 추가
    ALPHA_1, ALPHA_2, ALPHA_3, ALPHA_4,
    ALPHA_5, ALPHA_6, ALPHA_7, ALPHA_8,
    ALPHA_9, ALPHA_0,

    SPACE,
    ENTER,
    ESC,
    TAB,
    SHIFT,
    CTRL,

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

    // 새로운 그룹 타입들 추가
    ITEM,           // 아이템
    TILE,           // 타일/지형
    SPECIAL,        // 특수 오브젝트 (문, 스위치 등)
    UI,             // UI 요소

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

// 에디터 모드 열거형 (확장)
enum class EDITOR_MODE
{
    NONE,           // 기본 모드
    PLACE_MONSTER,  // 몬스터 배치 모드
    PLACE_ITEM,     // 아이템 배치 모드
    PLACE_TILE,     // 타일 배치 모드
    PLACE_SPECIAL,  // 특수 오브젝트 배치 모드
    SELECT,         // 선택 모드
    ERASE,          // 삭제 모드
    CAMERA_MOVE,    // 카메라 이동 모드
    END
};

// 오브젝트 타입 열거형 추가
enum class OBJECT_TYPE
{
    // 플레이어
    PLAYER,

    // 몬스터 타입들
    MONSTER_WADDLE_DEE,     // 와들디 (기본 적)
    MONSTER_GORDOS,         // 고르도스 (가시 적)
    MONSTER_BRONTO_BURT,    // 브론토 버트 (날아다니는 적)
    MONSTER_HOT_HEAD,       // 핫 헤드 (불 적)

    // 아이템 타입들
    ITEM_STAR,              // 별 (기본 아이템)
    ITEM_ENERGY_DRINK,      // 에너지 드링크 (체력 회복)
    ITEM_1UP,               // 1UP 아이템
    ITEM_ABILITY_STAR,      // 능력 별

    // 타일/환경 오브젝트
    TILE_GROUND,            // 일반 땅
    TILE_SPIKE,             // 가시 타일
    TILE_WATER,             // 물 타일
    TILE_WARP_STAR,         // 워프 스타

    // 특수 오브젝트
    OBJECT_DOOR,            // 문
    OBJECT_SWITCH,          // 스위치
    OBJECT_MIRROR,          // 거울 (게임 제목에 맞게)

    END
};
