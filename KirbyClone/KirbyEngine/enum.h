#pragma once

enum class KEY_STATE
{
    NONE ,   // 이전에도 안눌림, 지금도 안눌림
    TAP ,    // 이전에 안눌림, 지금 눌림
    HOLD ,   // 이전에도 눌림, 지금도 눌림
    AWAY ,   // 이전에 눌림, 지금 안눌림
};

enum class KEY
{
    LEFT ,
    RIGHT ,
    UP ,
    DOWN ,

    Q , W , E , R , T , Y , U , I , O , P ,
    A , S , D , F , G , H , J , K , L ,
    Z , X , C , V , B , N , M ,

    // 숫자 키 추가
    ALPHA_1 , ALPHA_2 , ALPHA_3 , ALPHA_4 ,
    ALPHA_5 , ALPHA_6 , ALPHA_7 , ALPHA_8 ,
    ALPHA_9 , ALPHA_0 ,

    ALT ,      // Alt 키
    HOME ,     // Home 키
    BACK ,     // Backspace 키

    SPACE ,
    ENTER ,
    ESC ,
    TAB ,
    SHIFT ,
    CTRL ,

    MOUSE_LEFT ,
    MOUSE_RIGHT ,
    MOUSE_MIDDLE ,

    LAST ,  // enum의 끝
};

enum class GROUP_TYPE
{
    DEFAULT ,
    PLAYER ,
    MONSTER ,
    PROJ_PLAYER ,    // 플레이어 투사체
    PROJ_MONSTER ,   // 몬스터 투사체

    // 새로운 그룹 타입들 추가
    ITEM ,           // 아이템
    TILE ,           // 타일/지형
    SPECIAL ,        // 특수 오브젝트 (문, 스위치 등)
    UI ,             // UI 요소
    EFFECT,

    END = 32 ,
};

enum class SCENE_TYPE
{
    START ,
    STAGE_01 ,
    STAGE_02 ,

    END ,
};

// 이벤트 타입 추가
enum class EVENT_TYPE
{
    // === 기존 기본 이벤트들 ===
    CREATE_OBJECT ,      // 오브젝트 생성
    DELETE_OBJECT ,      // 오브젝트 삭제
    SCENE_CHANGE ,       // 씬 변경
    COLLISION_ENTER ,    // 충돌 시작
    COLLISION_EXIT ,     // 충돌 종료

    // === 플레이어 관련 이벤트 ===
    PLAYER_INHALE_START ,    // 빨아들이기 시작
    PLAYER_INHALE_UPDATE ,   // 빨아들이기 진행 중
    PLAYER_INHALE_COMPLETE , // 빨아들이기 완료 (흡수)
    PLAYER_SPIT_OUT ,        // 뱉기
    PLAYER_SWALLOW ,         // 삼키기 (능력 획득)
    PLAYER_DAMAGE ,          // 플레이어 데미지
    PLAYER_DEATH ,           // 플레이어 사망
    PLAYER_SLIDE_KICK_RECOIL , // 슬라이딩킥 반동
    GAME_OVER ,              // 게임오버 (생명 감소)

    // === 몬스터 관련 이벤트 ===
    MONSTER_DAMAGE ,         // 몬스터 데미지
    MONSTER_DEATH ,          // 몬스터 사망
    MONSTER_ATTACK ,         // 몬스터 공격

    // === 보스 관련 이벤트 ===
    BOSS_BATTLE_START ,      // 보스전 시작
    BOSS_DAMAGE ,            // 보스 데미지
    STAGE_CLEAR ,            // 스테이지 클리어 (= 보스 패배)

    // === 능력 시스템 이벤트 ===
    ABILITY_ACQUIRE ,        // 능력 획득
    ABILITY_LOSE ,           // 능력 상실
    ABILITY_USE ,            // 능력 사용

    // === 투사체 관련 이벤트 ===
    PROJECTILE_FIRE ,        // 투사체 발사
    PROJECTILE_HIT ,         // 투사체 명중

    // === 아이템 관련 이벤트 ===
    ITEM_COLLECT ,           // 아이템 획득 (즉시 사용)

    // === 게임 진행 이벤트 ===
    STAGE_START ,            // 스테이지 시작
    DOOR_ENTER ,             // 문 통과

    // === 사운드/이펙트 이벤트 ===
    SOUND_PLAY ,             // 사운드 재생
    EFFECT_CREATE ,          // 이펙트 생성
    SCREEN_SHAKE ,           // 화면 진동
    FADE_COMPLETE ,          // 페이드 효과 완료

    // === 환경 관련 이벤트 ===
    BLOCK_BREAK ,            // 블록 파괴

    END
};

// 플레이어 상태 열거형
enum class PLAYER_STATE
{
    IDLE ,           // 기본 대기
    WALK ,           // 걷기
    RUN ,            // 뛰기  
    JUMP ,           // 점프
    FALL0 ,          // 낙하 초기 (점프 직후)
    FALL1 ,          // 낙하 (일반)
    FALL2 ,          // 낙하 (장시간 - 바운스 예정)
    BOUNCE ,         // 땅에서 바운스

    // === 크라우치 관련 상태들 ===
    CROUCH ,         // 크라우치 (앉기) - DOWN 키, 방향변경만 가능
    SLIDE ,          // 슬라이드 킥 (크라우치에서 Z키 또는 X키)
    SLIDE_KICK_RECOIL , // 슬라이드 킥 적중 후 반동

    // === 피격 상태 ===
    DAMAGE ,         // 데미지를 받는 상태

    // === 공중 부유 관련 상태들 ===
    HOVER ,          // 공기머금기 부유 (공중에서 A키)
    HOVER_EXHALE ,   // 내뱉기

    // === 빨아들이기 관련 상태들 ===
    INHALE ,         // 빨아들이는 중
    INHALE_KEEP ,    // 빨아들이기 지속 (X키 홀드)
    INHALE_SUCCESS , // 빨아들이기 성공 (INHALE과 MOUTHFUL_IDLE 사이)
    EXHALE ,         // 뱉기
    SWALLOW ,        // 삼키기

    // === 머금은 상태들 ===
    MOUTHFUL_IDLE ,  // 머금은 상태 대기
    MOUTHFUL_WALK ,  // 머금은 상태 걷기
    MOUTHFUL_RUN ,   // 머금은 상태 뛰기
    MOUTHFUL_JUMP ,  // 머금은 상태 점프
    MOUTHFUL_FALL ,  // 머금은 상태 낙하
    MOUTHFUL_DAMAGE ,// 머금은 상태 피격

    // === 공격 상태 ===
    ATTACK ,         // 능력 공격 (0.5초)
    ATTACK_HOLD ,    // 공격 홀드 (파이어/스파크 지속)

    DOOR_ENTER ,
    VICTORY_DANCE ,  // 승리 춤 (보스 격파 후)

    END
};

// 빨아들이기 관련 정보
enum class INHALE_COUNT
{
    NONE = 0,        // 아무것도 빨아들이지 않음
    ONE = 1,         // 1마리 빨아들임
    MULTIPLE = 2,    // 2마리 이상 빨아들임
};

// 카피 능력 타입
enum class COPY_ABILITY
{
    NONE,            // 능력 없음
    FIRE,            // 핫 헤드로부터
    BEAM,            // 웨이들 두로부터  
    SPARK,        // 스파키로부터
    
    END
};

// 몬스터 상태 열거형
enum class MONSTER_STATE
{
    IDLE ,
    WALK ,
    TURN ,

    // 새로 추가할 상태들
    DAMAGE ,         // 데미지를 받는 상태
    BEING_INHALED ,  // 빨아들려지는 상태
    ATTACK_READY ,   // 공격 준비 상태
    ATTACK ,         // 공격 상태
    FLY ,           // 비행 상태 (브론토 버트, 고르도)

    // 보스 패배 상태들
    DEFEAT1 ,        // 패배 애니메이션 1단계
    DEFEAT3 ,        // 패배 애니메이션 3단계

    // EDITOR_IDLE,     // 에디터 전용 상태 - 현재 미사용

    END
};

// 에디터 모드 열거형 (확장) - 현재 미사용, 나중에 에디터 구현시 활성화
/*
enum class EDITOR_MODE
{
    NORMAL ,             // 기본 모드
    SELECT ,             // 선택 모드
    ERASE ,              // 삭제 모드

    // 객체 배치 모드들
    PLACE_MONSTER ,      // 몬스터 배치
    PLACE_ITEM ,         // 아이템 배치
    PLACE_TILE ,         // 충돌체 배치
    PLACE_SPECIAL ,      // 특수 객체 배치

    // 환경 설정 모드들
    PLACE_STAGE ,        // 스테이지 이미지 선택 (새로 추가)
    BACKGROUND ,         // 배경 선택 (기존)

    // 기타 모드들
    PLAYER_SPAWN ,       // 플레이어 스폰 포인트 설정

    END
};
*/

// 오브젝트 타입 열거형 추가
enum class OBJECT_TYPE
{
    // 플레이어
    PLAYER ,

    // 몬스터 타입들
    MONSTER_WADDLE_DEE ,     // 웨이들 디 (기본 적)
    MONSTER_WADDLE_DOO ,     // 웨이들 두 (빔 공격)
    MONSTER_BRONTO_BURT ,    // 브론토 버트 (날아다니는 적)
    MONSTER_GORDOS ,         // 고르도 (가시 적)
    MONSTER_HOT_HEAD ,       // 핫 헤드 (불 적)
    MONSTER_SPARKY ,         // 스파키 (전기 적)

    // 보스
    MONSTER_WHISPY_WOODS ,

    // 아이템 타입들
    ITEM_STAR ,              // 별 (기본 아이템)
    ITEM_ENERGY_DRINK ,      // 에너지 드링크 (체력 회복)
    ITEM_1UP ,               // 1UP 아이템
    ITEM_ABILITY_STAR ,      // 능력 별

    // 기존 타일/충돌체 타입들
    TILE_GROUND ,        // 기존
    TILE_SPIKE ,         // 기존  
    TILE_WATER ,         // 기존
    TILE_WARP_STAR ,     // 기존

    // 새로 추가할 타일/충돌체 타입들
    TILE_PLATFORM ,      // 플랫폼 (위에서만 충돌)
    TILE_LAVA ,          // 용암 (데미지 + 통과)
    TILE_ONE_WAY ,       // 일방통행 플랫폼
    TILE_MOVING ,        // 움직이는 플랫폼
    TILE_BREAKABLE ,     // 부서지는 블록
    TILE_INVISIBLE ,     // 보이지 않는 벽
    TILE_TRIGGER ,       // 트리거 (보스전 시작 등)

    // 특수 오브젝트
    OBJECT_DOOR ,            // 문
    OBJECT_SWITCH ,          // 스위치
    OBJECT_MIRROR ,          // 거울 (게임 제목에 맞게)
    OBJECT_BACKGROUND ,      // 배경 오브젝트

    EFFECT,

    END
};

// 배경 타입 열거형
enum class BACKGROUND_TYPE
{
    STATIC ,             // 정적 배경 (스케일링만)
    SCROLLABLE ,         // 스크롤 가능한 배경
    PARALLAX ,           // 패럴랙스 스크롤 배경

    // 실제 배경 이미지들
    BACKGROUND1 ,        // background1.bmp
    BACKGROUND2 ,        // background2.bmp
    BACKGROUND3 ,        // background3.bmp

    END
};

// 충돌체 타입 (기존 TILE_VISUAL_TYPE 대체)
enum class COLLISION_TYPE
{
    SOLID_GROUND ,        // 파란색 - 기본 땅 (단단한 충돌)
    PLATFORM ,            // 초록색 - 플랫폼 (위에서만 충돌)
    SPIKE ,               // 빨간색 - 가시 (데미지 + 충돌)
    WATER ,               // 연파란색 - 물 (통과 가능, 특수 효과)
    LAVA ,                // 주황색 - 용암 (데미지 + 충돌)
    ONE_WAY_PLATFORM ,    // 연초록색 - 일방통행 플랫폼
    MOVING_PLATFORM ,     // 보라색 - 움직이는 플랫폼
    BREAKABLE_BLOCK ,     // 황토색 - 부서지는 블록
    INVISIBLE_WALL ,      // 회색 - 보이지 않는 벽
    TRIGGER ,             // 마젠타색 - 트리거 (통과 가능, 이벤트 발생)

    END
};


enum class TILE_VISUAL_TYPE
{
    TRANSPARENT_BLOCK ,      // 투명 충돌 블록 (기본)
    BOSS_TRIGGER ,           // 보스전 시작 트리거

    // === 향후 구현 예정 (주석처리) ===
    // GRASS_PLATFORM,
    // DIRT_BLOCK,            
    // STONE_BLOCK,           
    // GRASS_BLOCK,           
    // 
    // TREE,                  
    // FLOWER,                
    // FENCE,                 
    // PIPE,                  
    // 
    // SPIKE,                 
    // LAVA,                  
    // WATER,                 
    // 
    // MOVING_PLATFORM,       
    // BRIDGE,                

    END
};

enum class HOVER_SUBSTATE
{
    ENTER ,          // 최초 진입 (공기 머금기)
    FLY_UP ,         // Z키 입력으로 위로 올라가기 (버둥거리기)
    FLOAT ,          // 천천히 낙하 중
    GROUNDED ,       // 땅 위에서 떠다니기

    END
};

enum class PROJECTILE_TYPE
{
    // === 커비 전용 투사체 ===
    KIRBY_AIR_PUFF,         // 기본 공기 뱉기 (HOVER_EXHALE)
    KIRBY_SLIDE_KICK,       // 슬라이딩 킥 공격 (투명 투사체)
    KIRBY_STAR,             // 적 1마리 삼키고 뱉기
    KIRBY_STAR_ENHANCED,    // 적 2마리 이상 삼키고 뱉기 (강화된 별)
    KIRBY_FIRE,             // 파이어 능력 투사체
    KIRBY_BEAM,             // 웨이들두 카피 능력 (빔)
    KIRBY_ELECTRIC_FIELD,   // 스파키 카피 능력 (전기장)

    // === 몬스터 전용 투사체 ===
    BOSS_AIR_PUFF,          // 위스피 우드 공기포
    MONSTER_FIREBALL,       // 핫헤드 화염구
    MONSTER_ELECTRIC,       // 스파키 전기구슬
    MONSTER_BEAM,           // 웨이들두 빔

    END
};