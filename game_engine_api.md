# 게임 엔진 API 문서 (Updated)

> Kirby-like 2D 게임 프레임워크의 핵심 API/모듈을 빠르게 파악하고 바로 통합할 수 있도록 정리했습니다.  
> 본 문서는 **D3D11 기반** 구현을 포함하지만, 상위 레이어는 렌더러 추상화(`IRenderer`)를 통해 의존성을 최소화합니다.

---

## 목차

- [게임 엔진 API 문서 (Updated)](#게임-엔진-api-문서-updated)
  - [목차](#목차)
  - [통합 사용 순서 (권장 파이프라인)](#통합-사용-순서-권장-파이프라인)
  - [1) 기초 시스템](#1-기초-시스템)
    - [렌더링 시스템](#렌더링-시스템)
      - [`engine::IRenderer` \& `engine::D3D11Renderer`](#engineirenderer--engined3d11renderer)
      - [`engine::RenderSystem` (고수준 오케스트라)](#enginerendersystem-고수준-오케스트라)
      - [`engine::D3D11SpriteBatch`](#engined3d11spritebatch)
      - [`engine::D3D11DebugDraw`](#engined3d11debugdraw)
      - [`engine::D3D11SpriteRenderer`](#engined3d11spriterenderer)
    - [리소스 로딩](#리소스-로딩)
      - [`engine::TextureLoader` \& `engine::Tex2D`](#enginetextureloader--enginetex2d)
    - [타이밍 \& 입력](#타이밍--입력)
      - [`engine::Time`](#enginetime)
      - [`engine::Input`](#engineinput)
  - [2) 게임 월드 구성](#2-게임-월드-구성)
    - [타일/맵 시스템](#타일맵-시스템)
      - [`engine::TileSet`](#enginetileset)
      - [`engine::TileMap`](#enginetilemap)
      - [`engine::WorldSystem`](#engineworldsystem)
    - [충돌 \& 물리](#충돌--물리)
      - [`engine::physics::CollisionSystem`](#enginephysicscollisionsystem)
      - [`engine::PhysicsBody`](#enginephysicsbody)
    - [카메라](#카메라)
      - [`engine::Camera`](#enginecamera)
  - [3) 게임 오브젝트](#3-게임-오브젝트)
    - [기본 클래스](#기본-클래스)
    - [플레이어](#플레이어)
    - [몬스터](#몬스터)
    - [프로젝타일](#프로젝타일)
    - [애니메이션](#애니메이션)
  - [4) 데이터 로딩](#4-데이터-로딩)
    - [CSV 로더 (`StageCSV`)](#csv-로더-stagecsv)
  - [구현/사용 상 주의점 요약](#구현사용-상-주의점-요약)
  - [통합 사용 예시 (한 눈에)](#통합-사용-예시-한-눈에)
  - [확장/개선 포인트](#확장개선-포인트)
  - [최근 조정 사항(요약)](#최근-조정-사항요약)

---

## 통합 사용 순서 (권장 파이프라인)

1) **플랫폼/렌더러 초기화**
- `D3D11Renderer.Initialize(hwnd, w, h, vsync)`  
- `RenderSystem.Init(&renderer)` → `RenderSystem.OnResize(w, h)`  
- (텍스처 로딩 예정이면) 앱 시작 시 1회: `CoInitializeEx(nullptr, COINIT_MULTITHREADED)`

2) **월드/타일/충돌**
- `WorldSystem.LoadTileset(dev, L"tiles.png", tileW, tileH)`  
- CSV 또는 코드로 타일 정의 주입: `WorldSystem.DefineTile(id, TileDef{ solid, oneway, src })`  
- `WorldSystem.LoadMapCSV(L"map.csv")` → `WorldSystem.RebuildColliders()`  
- 카메라 월드 경계: `cam.SetWorldRect(world.WorldRectPx())`

3) **CSV로 스테이지 요소 로드**
- `LoadPlayerStartCSV("player.csv", out)`  
- `LoadMonstersCSV("monsters.csv", mons)` → `MonsterFactory::Create(...)`로 스폰  
- `LoadTileDefsCSV("tiles.csv", defs)` → 규칙에 따라 `DefineTile` 반영

4) **플레이어/몬스터/프로젝타일 구성**
- `PlayerFSM.Init(&player.Body(), &world.Collision(), player.Animator(), cfg)`  
- 몬스터 생성 후 콜백: `SetProjectileSpawner(...)`, `SetTargetQuery(...)` 등

5) **카메라**
- `cam.SetScreenSize(w, h)` → `cam.SetPixelSnap(true)` → 스폰 직후 `cam.SnapImmediate()`  
- 매 프레임: `cam.SetLookAt(player.Center()); cam.Update(dt)`

6) **메인 루프**
```cpp
time.TickFrame();
input.BeginFrame();

// 스파이럴 방지(상한): 앱 레벨에서 처리 권장
int steps = 0;
constexpr int MAX_STEPS = 5;
while (time.ShouldFixedUpdate() && steps < MAX_STEPS) {
    FixedUpdate(time.FixedDelta()); // FSM/물리/충돌 등
    time.ConsumeFixedStep();
    ++steps;
}

// 렌더
rs.Begin({0,0,0,1});
auto [ox,oy] = cam.OffsetInt();
world.RenderVisible(rs.Batch(), ox, oy, rs.BackbufferWidth(), rs.BackbufferHeight());
// 스프라이트/디버그 추가 드로우...
rs.End();
```

---

## 1) 기초 시스템

### 렌더링 시스템

#### `engine::IRenderer` & `engine::D3D11Renderer`
- **역할**
  - IRenderer: 엔진 상위 레이어가 의존하는 최소 추상 인터페이스.
  - D3D11Renderer: DXGI/D3D11 디바이스/컨텍스트/스왑체인 생성 및 RTV/뷰포트 관리.
- **핵심 API**
  - `Initialize(void* hwnd, int w, int h, bool vsync)` / `Resize(w, h)`  
  - `BeginFrame(Color clear)` / `EndFrame()`
  - **추가(추상화 누수 제거용):**
    - `BackbufferSize GetBackbufferSize() const`  
    - `bool GetD3D11Handles(ID3D11Device** dev, ID3D11DeviceContext** ctx)` (기본 false, D3D11Renderer에서 true 반환)
- **팁**
  - IRenderer 헤더는 `ID3D11Device`/`ID3D11DeviceContext` **전방 선언만** 사용(헤더 간 결합 최소화).

#### `engine::RenderSystem` (고수준 오케스트라)
- **역할**: 프레임 Begin/End, 카메라 변환(+줌), 스프라이트 배치/디버그 드로우 브릿지.
- **주요 API**
  - `bool Init(IRenderer* r)`, `void OnResize(w,h)`
  - `void Begin(Color clear)`, `void End()`
  - `void SetCamera(const Camera*)`, `std::pair<float,float> ToScreen(wx,wy)`
  - `void DrawSprite(const Tex2D&, wx,wy,w,h, src, rgba, rot, ox,oy, z=0, sort=0)`
  - `D3D11SpriteBatch& Batch()`, `D3D11DebugDraw& Debug()`
  - `int BackbufferWidth() const`, `int BackbufferHeight() const`
- **동작**
  - Init에서 `IRenderer->GetD3D11Handles` & `GetBackbufferSize`로 내부 배치/디버그 초기화.
  - Begin: 렌더러 클리어 → 배치/디버그 Begin → 상태 캐시 리셋.
  - End: 배치 End(플러시) → 디버그 Flush → Present.

#### `engine::D3D11SpriteBatch`
- **역할**: Draw 호출을 수집→정렬→그룹화하여 텍스처/블렌드/샘플러 전환을 최소화.
- **API**
  - `Initialize(dev, ctx, screenW, screenH)`, `OnResize(w,h)`
  - `Begin()` → `Draw(...)`(v1/v2) → `End()`
  - v2 `Draw`: `(..., int16_t zSort, BlendMode blend=Alpha, SamplerMode sampler=Point)`
- **정렬 키**
  - hi: `[blend:8 | sampler:8 | z:16 | pad:32]` / lo: SRV 포인터
- **주의**
  - 좌표는 **스크린 픽셀 공간**. 월드→스크린은 상위(`RenderSystem`)에서 처리.

#### `engine::D3D11DebugDraw`
- **역할**: 라인/AABB 디버그 드로우. 월드/스크린 헬퍼 제공.
- **API**: `Initialize`, `OnResize`, `BeginFrame`, `Line/Rect/WorldLine/WorldRect`, `Flush()`

#### `engine::D3D11SpriteRenderer`
- **역할**: 사각형 1개 즉시 드로우(툴/실험용). 동적 VB(4), 고정 IB(6).

---

### 리소스 로딩

#### `engine::TextureLoader` & `engine::Tex2D`
- **역할**
  - `LoadTextureWIC`: 파일을 32bpp RGBA(Non-premultiplied)로 로드→불변 텍스처+SRV 생성.
  - `CreateSolidTexture1x1`: 1×1 단색 텍스처.
- **주의**
  - 사용 전 `CoInitializeEx` 필요.
  - 블렌딩 상태는 Non-premultiplied에 맞춰 구성되어 있음.

---

### 타이밍 & 입력

#### `engine::Time`
- **역할**: 60Hz 고정 업데이트/가변 렌더델타, FPS 계산.
- **패턴**
  - 앱 레벨에서 **스파이럴 방지 상한**을 적용(예: 5회).

#### `engine::Input`
- **역할**: 키/마우스 폴링 + 액션/축 매핑.  
- **API 요약**
  - `Init(HWND)`, `BeginFrame()`, `OnWndMessage(...)`
  - 키: `Down/Pressed/Released(vk)`
  - 마우스: `MousePos/MouseDelta`, `ConsumeWheel()`
  - 액션/축: `BindAction/BindAxis`, `Action*`, `GetAxis(name) -> [-1,1]`

---

## 2) 게임 월드 구성

### 타일/맵 시스템

#### `engine::TileSet`
- **역할**: 타일 아틀라스 텍스처 + id→`TileDef`(solid/oneway/src) 보관.
- **API**: `LoadAtlas`, `Define`, `Get`, `TileW/H`, `Atlas()`

#### `engine::TileMap`
- **역할**: CSV 맵 로드, SOLID를 큰 직사각형으로 병합하여 충돌 생성, 가시 타일만 렌더.
- **API**: `LoadCSV`, `BuildSolidColliders(cs, tiles)`, `Render(batch, tiles, camOffX, camOffY, screenW, screenH)`
- **좌표계**: 월드(px) = 타일 인덱스 × (TileW, TileH)

#### `engine::WorldSystem`
- **역할**: `TileSet`+`TileMap`+`CollisionSystem` 오케스트라.
- **API**: `Load(...)`, `LoadTileset`, `LoadMapCSV`, `RebuildColliders`, `RenderVisible`, `WorldRectPx()`

---

### 충돌 & 물리

#### `engine::physics::CollisionSystem`
- **역할**: 정적/원웨이 박스 보관 및 `MoveAndCollide`로 AABB 충돌 처리(+원웨이 스냅/히스테리시스).
- **API**
  - `AddStaticBox/AddOneWayBox`, `MoveAndCollide(RECT& aabb, Vec2& vel, CollisionReport*, ignoreOneWay, prevBottom)`
  - `Statics()/OneWays()` 접근자, `DebugDraw(...)`
- **특징**
  - 원웨이 조건과 스냅(1px)은 현재 하드코딩(필요 시 파라미터화 권장).

#### `engine::PhysicsBody`
- **역할**: 러닝 가속/감속/마찰/중력/종단속도, 점프/임펄스, 충돌 보정 반영.
- **API**
  - 입력: `SetDesiredRunAxis(ax)`/`SetDesiredRunSpeedX(vx)`
  - 적분: `AdvanceKinematics(dt)` → (충돌 전) `ProposeAABB(dt, &prevBottom, &nx, &ny)` → (충돌 결과) `ApplyCollisionResult(...)`
  - 간이 플레이: `IntegrateAndClampNoCollision(dt)`
  - 쿼리: `GetBounds/BoundsRect`, `Velocity`, `Grounded`, `Params()`

---

### 카메라

#### `engine::Camera`
- **역할**: 화면 크기/월드 경계, 목표 위치 스무딩, 픽셀 스냅, 화면 흔들림.
- **API**
  - 화면/월드: `SetScreenSize(w,h)`, `SetWorldRect(l,t,r,b|RECT)`
  - 스무딩/스냅: `SetSmoothSpeed(k)`, `SetPixelSnap(on)`
  - 추적: `SetLookAt(Vec2)`, `Update(dt)`, `SnapImmediate()`
  - 변환: `OffsetInt() -> (ox, oy)`, `Current()`, `GetLookAt()`
- **팁**: 픽셀 아트는 `SetPixelSnap(true)` 권장.

---

## 3) 게임 오브젝트

### 기본 클래스
- `engine::Object`: `Update(double fixedDt, const Input&)`, `Render(HDC, ox, oy)` 가상 메서드.

### 플레이어
- `game::Player`: `PhysicsBody`, `Animator`, `Tex2D` 보유.  
- `game::PlayerFSM`: HFSM(Idle/Walk/Jump/Fall/Damaged/Dead), 코요테/버퍼 점프/저점프/원웨이 드롭/지면 히스테리시스/점프락.  
  - `Init(PhysicsBody*, CollisionSystem*, Animator*, Cfg)`  
  - `Step(fixedDt, input)` / `ApplyDamage(Damage)`  
  - `DebugInfo` 스냅샷 제공.

### 몬스터
- `game::Monster` (베이스): `PhysicsBody`, `Animator`, `Health`, `StepPhysics` 공통, 히트/넉백/무적 처리.  
  - 오버라이드 지점: `TickAI(fixedDt, input)`  
  - 유틸: `HasGroundAhead(dir)`, `SetProjectileSpawner`, `SetTargetQuery`
- 샘플
  - `WaddleDee`: 단순 좌우 순찰(벽/절벽에서 방향 전환 옵션).
  - `WaddleDoo`: 순찰 + 사격(감지/윈드업/쿨다운/탄속 파라미터).

### 프로젝타일
- `game::Projectile`: TTL, 직진(중력 옵션), 세계 충돌 시 소멸(옵션).

### 애니메이션
- `engine::Animator`: 클립 루프/단발/행렬적 프레임 전진, 문자열 클립 이름.  
  - 유틸: `MakeRowClip`로 일정 간격 스프라이트시트에서 연속 프레임 생성.

---

## 4) 데이터 로딩

### CSV 로더 (`StageCSV`)
- `LoadPlayerStartCSV(path, out)`  
- `LoadMonstersCSV(path, out)` — 필드 미지정은 `-1`/기본값으로 처리.  
- `LoadTileDefsCSV(path, out)` — `id/solid/oneway` 읽어옴. 아틀라스 src 매핑은 별도 규칙 필요.

---

## 구현/사용 상 주의점 요약

- **타이밍/스파이럴**: `Time`은 누적 버퍼만 제공. **상한(예: 5회)** 은 앱 레벨(메인 루프)에서 적용.  
- **입력**: 매 프레임 `BeginFrame()` 필수. `OnWndMessage`를 WndProc에서 전달해야 휠·포커스 정상 동작.  
- **카메라**: 픽셀 스냅 켜면 `OffsetInt()`가 픽셀 정렬. 줌은 화면 중심 기준 스케일.  
- **충돌**: One-way/스냅 EPS는 하드코딩(1px). 스테이지에 따라 튜닝 가능하도록 파라미터화 권장.  
- **리소스 로딩**: WIC 사용 전 `CoInitializeEx`. 알파는 Non-premultiplied 규약.  
- **렌더 추상화**: `RenderSystem`은 `IRenderer.GetD3D11Handles()`/`GetBackbufferSize()`로 초기화. D3D11 외 백엔드 도입 시 해당 메서드만 구현하면 연동 가능.  
- **Animator 문자열**: 오타로 인한 런타임 오류 방지 위해 상수/enum 매핑 유틸 고려.  
- **Health/Damage**: PlayerFSM/Monster 양쪽에서 사용 — 공통 파라미터(경직/넉백 한계) 정리 권장.  
- **MonsterFactory**: 물리 프리셋 DRY — 공통 기본값 + 차이만 오버라이드.

---

## 통합 사용 예시 (한 눈에)

```cpp
// 초기화
D3D11Renderer renderer;
renderer.Initialize(hwnd, w, h, true);

engine::RenderSystem rs;
rs.Init(&renderer);
rs.OnResize(w, h);

engine::WorldSystem world;
world.LoadTileset(renderer.Device(), L"tiles.png", 16, 16);
// (CSV/규칙 기반) tiles.Define(...) 채움
world.LoadMapCSV(L"map.csv");
world.RebuildColliders();

engine::Camera cam;
cam.SetScreenSize(w, h);
cam.SetWorldRect(world.WorldRectPx());
cam.SetPixelSnap(true);

// 플레이어/FSM
game::Player player(world.WorldRectPx(), 100, 100);
game::PlayerFSM fsm;
fsm.Init(&player.Body(), &world.Collision(), player.Animator(), {});
cam.SetLookAt(player.Center());
cam.SnapImmediate();

// 루프
for (;;) {
  time.TickFrame();
  input.BeginFrame();

  // 고정 업데이트
  int steps=0; const int MAX_STEPS=5;
  while (time.ShouldFixedUpdate() && steps++ < MAX_STEPS) {
    fsm.Step(time.FixedDelta(), input);
    // monsters/projectiles ... Step
    time.ConsumeFixedStep();
  }

  // 카메라
  cam.SetLookAt(player.Center());
  cam.Update(time.DeltaTime());

  // 렌더
  rs.Begin({0.1f,0.12f,0.16f,1});
  auto [ox,oy] = cam.OffsetInt();
  world.RenderVisible(rs.Batch(), ox, oy, rs.BackbufferWidth(), rs.BackbufferHeight());
  // 추가 드로우...
  rs.End();
}
```

---

## 확장/개선 포인트

- **Collision broadphase**: 현재 정적 박스 전수 검사. 타일 버킷/셀 그리드(간단) 또는 스윕/분할 구조를 도입하면 대형 맵 성능↑.  
- **원웨이/스냅 파라미터화**: `CollisionSystem`/`PhysicsParams`에 EPS/정책 노출.  
- **Animator 상태 유틸**: 문자열 대신 enum → 이름표 테이블 도입.  
- **공통 프리셋/튜닝 DRY**: Monster 물리 파라미터 공통화.  
- **Render 추상화**: D3D11 외 백엔드(예: SDL+GL) 추가 시에도 `IRenderer` 계약만 만족하면 `RenderSystem` 재사용 가능.

---

## 최근 조정 사항(요약)

- `IRenderer`에 **전방 선언 기반** D3D11 핸들 게터 추가:  
  - `bool GetD3D11Handles(ID3D11Device**, ID3D11DeviceContext**)` (기본 false)  
  - `BackbufferSize GetBackbufferSize() const`  
  - → `RenderSystem`에서 `dynamic_cast` 제거, **추상화 누수 축소**.
- `RenderSystem` 초기화 경로 갱신: `IRenderer`만으로 내부 배치/디버그 초기화 & 크기 질의.
- 문서 전체 보강: 통합 순서/주의점/예시/개선 포인트 정리.

---