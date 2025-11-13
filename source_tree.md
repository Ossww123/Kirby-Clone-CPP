```
/protocol
  SaveSchema.h — 게임/엔진 전 계층이 공유하는 세이브 데이터 스키마와 유틸(계약 계층).
/engine
  /core
    GameApp.* — 앱 루프·입력/타이밍 관리·렌더/세션 초기화와 오케스트레이션
    RenderSystem.* — 프레임 시작/종료, 카메라 변환, 배치·디버그 드로우 오케스트레이션
    Scene.* — 오브젝트 컨테이너: 생성/업데이트/정리.
    Time.* — 프레임 타이밍(가변 dt + 고정 스텝) 및 FPS 카운터(QPC 기반).
    Input.* — Win32 폴링 입력(키보드/마우스), 에지 플래그, 단순 액션/축 매핑.
    Object.h — 기본 오브젝트: Id + Update 훅.
  /render
    IRenderer.h — 렌더러 공통 인터페이스(초기화·리사이즈·프레임 관리, 백버퍼 크기/핸들 질의).
    IDebugDraw.h — 렌더러 무관 디버그 드로우 인터페이스(월드라인/사각형)
    D3D11DebugDrawAdapter.* — D3D11 디버그 드로우를 IDebugDraw로 어댑트
    D3D11Renderer.* — D3D11 스왑체인·RTV·뷰포트 제어를 제공하는 렌더러 클래스 선언. 디바이스/스왑체인 생성, 프레임 시작/종료, 리사이즈 구현.
    D3D11Sprite.* — D3D11 단일 사각형 스프라이트 드로우(픽셀 투영, IntRect 소스, RGBA8 틴트)
    D3D11SpriteBatch.* — D3D11 스프라이트 일괄 렌더(정렬/버퍼 업로드/상태 버킷), RECT·IntRect 모두 지원
    Texture.h — D3D11 SRV와 픽셀 크기를 보관하는 경량 2D 텍스처 핸들.
    TextureLoader.h — WIC 파일 로드·1x1 단색 생성 등을 제공하는 Tex2D 로드 API.
    D3D11TextureLoader.cpp — WIC으로 RGBA8 디코드 후 D3D11 텍스처/SRV 생성 구현.
    DWriteText.h — D3D11 스왑체인 위에 D2D/DirectWrite로 텍스트 HUD(엔진 Color, 가벼운 헤더)
    D3D11DebugDraw.* — 선/사각형 디버그 오버레이(D3D11 라인리스트, 알파 블렌딩).
  /world
    Camera.h — 경계 클램프/픽셀 스냅/감쇠형 화면 흔들림을 지원하는 2D 추적 카메라.
    TileSet.* — 타일 아틀라스/셀 슬라이싱 및 타일 메타데이터 관리(IntRect 기반)
    TileMap.* — 타일 ID 그리드 보관/가시 영역 렌더링 및 정적·원웨이 콜라이더 생성
    WorldSystem.* — 타일셋·타일맵·충돌을 묶어 관리하고, 가시 타일 렌더링/월드 픽셀 크기 제공
  /physics
    AABB.h — 2D 충돌 유틸 확장(AABB·원·선분·캡슐)
    Collision.* — 정적/원웨이 충돌 해결 + 스냅 파라미터화
    CollisionDebugDraw.* — 충돌 시스템 와이어 드로우 어댑터(D3D11DebugDraw 사용)
    PhysicsBody.* — 캐릭터용 가속/마찰/중력과 AABB 제안·적용(월드 클램프 보조)
  /util
    Anim.* — 경량 스프라이트 애니메이션(클립/프레임), 시간 진행; 렌더링 없음.
    Math.h — Vec2 및 기본 수학 상수/변환(헤더 온리).
    StringConv.h — UTF-8↔UTF-16 헬퍼(Win32 MultiByte/WideChar 사용); 헤더에서 Win32 의존 감춤.
    Types.h — 엔진 공용 IntRect 및 간단 헬퍼(헤더온리).
  /save
    SaveStorage.* — 세이브 데이터 파일을 간단한 k=v 텍스트로 로드/세이브하는 엔진 I/O 계층.
  /platform/win32
    RectUtil.h — Win32 RECT ↔ IntRect 변환 인라인 어댑터.
    ColorUtil.h — COLORREF↔RGBA8 변환 유틸;

/game
  /session
    PlaySession.h — 세션 런타임 오케스트레이션(월드·카메라·플레이어 FSM·전투·디버그/HUD 렌더)
    PlaySession.Core.cpp — 세션 초기화·고정틱 코어 로직(플레이어/몬스터/전투/카메라/페이드)
    PlaySession.Render.cpp — 세션 렌더 경로(패럴럭스 BG·월드·디버그·HUD·페이드)
    PlaySession.Stage.cpp — 스테이지 로드·월드 재구성(타일·배경·플레이어 시작·카메라 경계·몬스터/도어 스폰)
    PlaySession.Combat.cpp — 전투 시스템 초기화·타깃 빌드·플레이어 이벤트 처리·데미지 적용·런타임 스폰
    PlaySession.Door.cpp — 문 오버랩 감지 및 페이드 기반 스테이지 전환 상태 머신
    PlaySession.ClearFlow.cpp — 클리어 뒤 연출→세이브→허브 전환까지 “한 FSM”에서 책임지고, 세이브 타이밍을 페이드 아웃 완료 시점으로 고정.
    SessionState.* — 현재 슬롯과 세이브 데이터를 보관·저장하는 런타임 세션 컨테이너.
    SpawnSelector.* — 전환 오버라이드/세이브/스폰 테이블 기반으로 “이번에 어디서 시작할지”를 결정.
    HubCoverUnlock.h — 세이브 플래그를 읽어 허브 커버 타일을 지우고 충돌을 갱신.

  /data
    StageDesc.* — 스테이지 정보(JSON) 로더.
    StageCSV.* — 스테이지 CSV 로더(플레이어 시작/몬스터/타일정의/문/타일맵/문/허브-스폰/허브-언락).
    AnimCSV.* — 애니메이션 CSV( strip/frame ) 파싱 후 Animator에 클립 등록(옵션 초기화 지원)
    GameConfig.h — 전역 해상도/스케일 상수와 기본 월드 단위(타일/충돌 AABB) 정의
    StagePath.h — stage.json 경로를 만들어줌

  /entities
    /player
      Player.* — 플레이어 물리/애니/텍스처 핸들(헤더 IntRect·RGBA8만 노출, 로직은 FSM 담당)
      PlayerFSM.h — 플레이어 FSM 선언(3트랙 상태/이벤트/튜닝 상수)
      PlayerFSM.Core.cpp — FSM 코어(초기화/스텝/충돌적분/전이/스냅샷)
      PlayerFSM.AState.cpp — 플레이어 액션 상태 로직(흡입/입가득/별뱉기/공기포/능력공격).
      PlayerFSM.MState.cpp — 플레이어 이동 상태 로직(Idle/Walk/Run/Crouch/Slide/Jump/Fall/Inflated/Ladder).
      PlayerFSM.ZState.cpp — 플레이어 오버레이 상태 로직(피격/사망/문 입장/댄스/게임오버 및 상호작용).
      PlayerFSM.OverlayControl.cpp - 플레이어 오버레이(ZState) 상태를 전환하는 외부 api.
    /monsters
      Monster.h - 몬스터 베이스(물리/충돌·체력/넉백·스폰 훅, 헤더는 IntRect/RGBA8, 렌더디버그는 cpp)
      MonsterTypes.h — 몬스터 베이스(물리/충돌·체력/넉백·스폰 훅)
      MonsterFactory.* — 몬스터 타입→Maker 레지스트리와 SpawnSpec/충돌 컨텍스트를 받아 인스턴스를 생성하는 팩토리.
      WaddleDee.* — 단순 순찰 AI(에지/벽 턴, 이동축 설정)
      WaddleDoo.* — 순찰 + 빔 공격 FSM(윈드업/쿨다운, 타깃 감지)
      HotHead.* — 순찰 + 화염 분사 프로젝타일 FSM(윈드업/분사/쿨다운)
      Sparky.* — 점프 기반 이동 + 스파크 오라 FSM(지상 시작, 휴식 타이머)
      Apple.* — 텔레그래프→낙하→단발 바운스→구르기 FSM
      WhispyWoods.* — 보스 AI(공기포 연사↔사과 낙하 교대, 정지형)

  /combat
    CombatTypes.h — 투사체 소유자 열거(피격 필터용, 기반형 고정)
    CombatTarget.h — 전투 시스템 공용 대상 정보(AABB·생존/플레이어 플래그·흡입/능력 선물 메타)
    Damage.h — 팀/피해종류·데미지/체력 유틸(경량 인라인, Vec2 의존)
    Ability.h — 능력 열거(확장 여지, 기반형 고정)
    HitVolume.* — 단일 히트 볼륨(수명·부착·형상·퍼타깃 쿨다운)과 페이로드/규칙 정의, 시스템용 게이팅 제공
    HitVolumeFactory.* — 히트 볼륨 프리셋 레지스트리(문자키→Cfg 등록/탐색/기본셋 제공)
    HitVolumeSystem.* — 히트 볼륨 수명/소유자 추적·충돌 판정·이벤트 방출(스폰/스텝/캡처·데미지/디스폰) + 디버그 드로우
    HitVolumeGeom.* — 히트볼륨 월드 형상 빌더(박스/원/캡슐), 전투 도메인 전용

  /projectile
    Projectile.* — 투사체 객체 인터페이스(물리 바디·소유자·수명·페이로드 관리)
    ProjectileFactory.* -— 투사체 아키타입 정의와 스폰 인터페이스. 레지스트리, CSV 로드, 인스턴스 생성
    ProjectileSystem.* 투사체 업데이트·충돌 판정·히트 이벤트 발행 인터페이스

  /debugdraw
    MonsterDebugDraw.* — 몬스터 디버그 드로우 어댑터
    HitVolumeDebugDraw.* — 히트 볼륨 디버그 드로우 어댑터
    ProjectileDebugDraw.* — 투사체 디버그 드로우 어댑터

  /frontend
    FrontFlow.* — 부팅~시작까지의 메뉴 상태머신.

  /render
    ZOrder.h — 레이어 인자 값들을 정의

  /effects
    Fade2D.h — 페이드 인/아웃 연출 유틸
```