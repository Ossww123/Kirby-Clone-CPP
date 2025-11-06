```
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
    AABB.h — AABB 겹침/MTV 유틸(정수 좌표)
    Collision.* — 정적/원웨이 충돌 해결 + 스냅 파라미터화
    CollisionDebugDraw.* — 충돌 시스템 와이어 드로우 어댑터(D3D11DebugDraw 사용)
    PhysicsBody.* — 캐릭터용 가속/마찰/중력과 AABB 제안·적용(월드 클램프 보조)
  /util
    Anim.* — 경량 스프라이트 애니메이션(클립/프레임), 시간 진행; 렌더링 없음.
    Math.h — Vec2 및 기본 수학 상수/변환(헤더 온리).
    StringConv.h — UTF-8↔UTF-16 헬퍼(Win32 MultiByte/WideChar 사용); 헤더에서 Win32 의존 감춤.
    Types.h — 엔진 공용 IntRect 및 간단 헬퍼(헤더온리).
  /platform/win32
    RectUtil.h — Win32 RECT ↔ IntRect 변환 인라인 어댑터.
    ColorUtil.h — COLORREF↔RGBA8 변환 유틸;

/game
  /session
    PlaySession.h
    PlaySession.Core.cpp
    PlaySession.Render.cpp
    PlaySession.Stage.cpp
    PlaySession.Combat.cpp
    PlaySession.Door.cpp

  /data
    StageDesc.h
    StageDesc.cpp
    StageCSV.h
    StageCSV.cpp
    AnimCSV.h
    AnimCSV.cpp
    GameConfig.h

  /entities
    /player
      Player.* — 플레이어 물리/애니/텍스처 핸들(헤더 IntRect·RGBA8만 노출, 로직은 FSM 담당)
      PlayerFSM.h
      PlayerFSM.cpp
    /monsters
      Monster.h - 몬스터 베이스(물리/충돌·체력/넉백·스폰 훅, 헤더는 IntRect/RGBA8, 렌더디버그는 cpp)
      MonsterTypes.h — 몬스터 베이스(물리/충돌·체력/넉백·스폰 훅)
      MonsterFactory.h
      MonsterDebugDraw.* — 몬스터 디버그 드로우 어댑터(바운딩/HP 바)
      WaddleDee.h
      WaddleDee.cpp
      WaddleDoo.h
      WaddleDoo.cpp
      HotHead.h
      HotHead.cpp
      Sparky.h
      Sparky.cpp
      Apple.h
      WhispyWoods.h

  /combat
    CombatTypes.h — 투사체 소유자 열거(피격 필터용, 기반형 고정)
    CombatTarget.h
    Damage.h — 팀/피해종류·데미지/체력 유틸(경량 인라인, Vec2 의존)
    Ability.h — 능력 열거(확장 여지, 기반형 고정)
    HitVolume.h
    HitVolume.cpp
    HitVolumeFactory.h
    HitVolumeFactory.cpp
    HitVolumeSystem.h
    HitVolumeSystem.cpp

  /projectile
    Projectile.h
    ProjectileFactory.h
    ProjectileFactory.cpp
    ProjectileSystem.h
    ProjectileSystem.cpp
```