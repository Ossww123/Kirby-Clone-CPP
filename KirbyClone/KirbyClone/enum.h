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

    ALT,      // Alt 키
    HOME,     // Home 키
    BACK,     // Backspace 키

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
    STAGE_01,
    STAGE_02,

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
    IDLE,           // 기본 대기
    WALK,           // 걷기
    RUN,            // 뛰기  
    JUMP,           // 점프
    FALL,           // 낙하
    BOUNCE,         // 땅에서 바운스
    INHALE_READY,   // 빨아들이기 준비 (공기머금기)
    INHALE_1,       // 빨아들이기 1단계
    INHALE_2,       // 빨아들이기 2단계
    INHALE_HOLD,    // 숨참 (빨아들이기 유지)
    EXHALE,         // 공기뱉기
    SWALLOW,        // 삼키기
    MOUTHFUL_IDLE,  // 머금은 상태 대기
    MOUTHFUL_WALK,  // 머금은 상태 걷기
    MOUTHFUL_RUN,   // 머금은 상태 뛰기
    MOUTHFUL_JUMP,  // 머금은 상태 점프
    END
};

// 몬스터 상태 열거형
enum class MONSTER_STATE
{
    IDLE,
    WALK,
    TURN,

    // 새로 추가할 상태들
    DAMAGE,         // 데미지를 받는 상태
    ATTACK_READY,   // 공격 준비 상태
    ATTACK,         // 공격 상태
    FLY,           // 비행 상태 (브론토 버트, 고르도)

    END
};

// 에디터 모드 열거형 (확장)
enum class EDITOR_MODE
{
    NORMAL,             // 기본 모드
    SELECT,             // 선택 모드
    ERASE,              // 삭제 모드

    // 객체 배치 모드들
    PLACE_MONSTER,      // 몬스터 배치
    PLACE_ITEM,         // 아이템 배치
    PLACE_TILE,
    PLACE_COLLISION,    // 충돌체 배치 (기존 PLACE_TILE에서 이름 변경)
    PLACE_SPECIAL,      // 특수 객체 배치

    // 환경 설정 모드들
    PLACE_STAGE,        // 스테이지 이미지 선택 (새로 추가)
    BACKGROUND,         // 배경 선택 (기존)

    // 기타 모드들
    PLAYER_SPAWN,       // 플레이어 스폰 포인트 설정
    CAMERA_MOVE,        // 카메라 이동

    END
};

// 오브젝트 타입 열거형 추가
enum class OBJECT_TYPE
{
    // 플레이어
    PLAYER,

    // 몬스터 타입들
    MONSTER_WADDLE_DEE,     // 웨이들 디 (기본 적)
    MONSTER_WADDLE_DOO,     // 웨이들 두 (빔 공격)
    MONSTER_BRONTO_BURT,    // 브론토 버트 (날아다니는 적)
    MONSTER_GORDOS,         // 고르도 (가시 적)
    MONSTER_HOT_HEAD,       // 핫 헤드 (불 적)
    MONSTER_SPARKY,         // 스파키 (전기 적)

    // 아이템 타입들
    ITEM_STAR,              // 별 (기본 아이템)
    ITEM_ENERGY_DRINK,      // 에너지 드링크 (체력 회복)
    ITEM_1UP,               // 1UP 아이템
    ITEM_ABILITY_STAR,      // 능력 별

    // 기존 타일/충돌체 타입들
    TILE_GROUND,        // 기존
    TILE_SPIKE,         // 기존  
    TILE_WATER,         // 기존
    TILE_WARP_STAR,     // 기존

    // 새로 추가할 타일/충돌체 타입들
    TILE_PLATFORM,      // 플랫폼 (위에서만 충돌)
    TILE_LAVA,          // 용암 (데미지 + 통과)
    TILE_ONE_WAY,       // 일방통행 플랫폼
    TILE_MOVING,        // 움직이는 플랫폼
    TILE_BREAKABLE,     // 부서지는 블록
    TILE_INVISIBLE,     // 보이지 않는 벽

    // 특수 오브젝트
    OBJECT_DOOR,            // 문
    OBJECT_SWITCH,          // 스위치
    OBJECT_MIRROR,          // 거울 (게임 제목에 맞게)

    END
};

// 배경 타입 열거형
enum class BACKGROUND_TYPE
{
    BACKGROUND1,        // background1.bmp
    BACKGROUND2,        // background2.bmp
    BACKGROUND3,        // background3.bmp

    END
};

// 충돌체 타입 (기존 TILE_VISUAL_TYPE 대체)
enum class COLLISION_TYPE
{
    SOLID_GROUND,        // 파란색 - 기본 땅 (단단한 충돌)
    PLATFORM,            // 초록색 - 플랫폼 (위에서만 충돌)
    SPIKE,               // 빨간색 - 가시 (데미지 + 충돌)
    WATER,               // 연파란색 - 물 (통과 가능, 특수 효과)
    LAVA,                // 주황색 - 용암 (데미지 + 충돌)
    ONE_WAY_PLATFORM,    // 연초록색 - 일방통행 플랫폼
    MOVING_PLATFORM,     // 보라색 - 움직이는 플랫폼
    BREAKABLE_BLOCK,     // 황토색 - 부서지는 블록
    INVISIBLE_WALL,      // 회색 - 보이지 않는 벽

    END
};

// 스테이지 이미지 타입
enum class STAGE_IMAGE_TYPE
{
    STAGE_01,           // 첫 번째 스테이지 (Green Hill 스타일)
    STAGE_02,           // 두 번째 스테이지 (Castle 스타일)
    CUSTOM,             // 사용자 커스텀 스테이지

    END
};

enum class TILE_VISUAL_TYPE
{
    GRASS_PLATFORM,        
    DIRT_BLOCK,            
    STONE_BLOCK,           
    GRASS_BLOCK,           

    TREE,                  
    FLOWER,                
    FENCE,                 
    PIPE,                  

    SPIKE,                 
    LAVA,                  
    WATER,                 

    MOVING_PLATFORM,       
    BRIDGE,                

    END
};