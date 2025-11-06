```
/engine
  /core
    GameApp.h
    GameApp.cpp
    RenderSystem.h
    RenderSystem.cpp
    Scene.h/.cpp — 오브젝트 컨테이너: 생성/업데이트/정리.
    Time.h/.cpp — 프레임 타이밍(가변 dt + 고정 스텝) 및 FPS 카운터(QPC 기반).
    Input.h/.cpp — Win32 폴링 입력(키보드/마우스), 에지 플래그, 단순 액션/축 매핑.
    Object.h — 기본 오브젝트: Id + Update 훅.
  /render
    IRenderer.h
    D3D11Renderer.h
    D3D11Sprite.h
    D3D11SpriteBatch.h
    D3D11SpriteBatch.cpp
    Texture.h
    TextureLoader.h
    D3D11TextureLoader.cpp
    DWriteText.h
    D3D11DebugDraw.h/.cpp — 선/사각형 디버그 오버레이(D3D11 라인리스트, 알파 블렌딩).
  /world
    Camera.h — 경계 클램프/픽셀 스냅/감쇠형 화면 흔들림을 지원하는 2D 추적 카메라.
    TileSet.h
    TileSet.cpp
    TileMap.h
    TileMap.cpp
    WorldSystem.h
    WorldSystem.cpp
  /physics
    Collision.h
    Collision.cpp
    PhysicsBody.h
    PhysicsBody.cpp
  /util
    Anim.h/.cpp — 경량 스프라이트 애니메이션(클립/프레임), 시간 진행; 렌더링 없음.
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
      Player.h
      PlayerFSM.h
      PlayerFSM.cpp
    /monsters
      Monster.h
      MonsterTypes.h
      MonsterFactory.h
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
    CombatTypes.h
    CombatTarget.h
    Damage.h
    Ability.h
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