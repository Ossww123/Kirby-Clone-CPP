# 게임 엔진 API 문서 (Updated)

> Kirby-like 2D 게임 프레임워크의 핵심 API/모듈을 빠르게 파악하고 바로 통합할 수 있도록 정리했습니다.  
> 본 문서는 **D3D11 기반** 구현을 포함하지만, 상위 레이어는 렌더러 추상화(`IRenderer`)를 통해 의존성을 최소화합니다.
> 이번 업데이트에서 **PlayerFSM(병렬 상태 머신)**, **Inflated 로직**, **Spit/AirPuff 이벤트 흐름**, **ProjectileFactory(데이터 드리븐 투사체)**, **Animator CSV 로딩**을 반영했습니다.

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
- [PlaySession (런타임 오케스트라)](#playsession-런타임-오케스트라)
  - [공개 API](#공개-api)
  - [라이프사이클 \& 처리 순서](#라이프사이클--처리-순서)
    - [1) Initialize](#1-initialize)
    - [2) LoadStage(jsonPath)](#2-loadstagejsonpath)
    - [3) FixedUpdate(fixedDt, input)](#3-fixedupdatefixeddt-input)
    - [4) Render](#4-render)
  - [이벤트 처리 매핑(핵심만)](#이벤트-처리-매핑핵심만)
  - [전투 시스템 초기화 포인트](#전투-시스템-초기화-포인트)
  - [문(도어)/전환](#문도어전환)
  - [디버그 드로우](#디버그-드로우)
  - [HUD](#hud)
  - [데이터 포맷 요약](#데이터-포맷-요약)
  - [GameApp 통합 예시](#gameapp-통합-예시)
  - [주의/팁](#주의팁)
  - [2) 게임 월드 구성](#2-게임-월드-구성)
    - [타일/맵 시스템](#타일맵-시스템)
      - [`engine::TileSet`](#enginetileset)
      - [`engine::TileMap`](#enginetilemap)
      - [`engine::WorldSystem`](#engineworldsystem)
      - [`game::StageCSV` (게임 레이어)](#gamestagecsv-게임-레이어)
    - [스케일 합의](#스케일-합의)
    - [충돌 \& 물리](#충돌--물리)
      - [`engine::physics::CollisionSystem`](#enginephysicscollisionsystem)
      - [`engine::PhysicsBody`](#enginephysicsbody)
    - [카메라](#카메라)
      - [`engine::Camera`](#enginecamera)
  - [3) 게임 오브젝트](#3-게임-오브젝트)
    - [기본 클래스](#기본-클래스)
    - [플레이어 FSM (병렬 트랙: Movement/Action/Overlay)](#플레이어-fsm-병렬-트랙-movementactionoverlay)
    - [몬스터](#몬스터)
    - [프로젝타일 시스템 (Projectile + Factory + System)](#프로젝타일-시스템-projectile--factory--system)
    - [히트 볼륨 시스템 (HitVolume + Factory + System)](#히트-볼륨-시스템-hitvolume--factory--system)
    - [애니메이션 \& CSV 로더](#애니메이션--csv-로더)
  - [4) 데이터 로딩](#4-데이터-로딩)
    - [CSV 로더 (`StageCSV`)](#csv-로더-stagecsv)
  - [구현/사용 상 주의점 요약](#구현사용-상-주의점-요약)
  - [통합 사용 예시 (핵심 스니펫)](#통합-사용-예시-핵심-스니펫)
  - [디버그/HUD/툴링](#디버그hud툴링)
  - [구현/사용 상 주의점 요약](#구현사용-상-주의점-요약-1)
  - [업데이트 변경 이력](#업데이트-변경-이력)

---

## 통합 사용 순서 (권장 파이프라인)

1. **플랫폼/렌더러 초기화**
   - `D3D11Renderer.Initialize(hwnd, w, h, vsync)`

- `RenderSystem.Init(&renderer)` → `RenderSystem.OnResize(w, h)`
- (텍스처 로딩 예정이면) 앱 시작 시 1회: `CoInitializeEx(nullptr, COINIT_MULTITHREADED)`

2. **전투 관련 초기 등록/초기화**

   - `ProjectileFactory::RegisterDefaults()` / `HitVolumeFactory::RegisterDefaults()` → `ProjectileSystem.Initialize(worldRect, &Collision)` / `HitVolumeSystem.Initialize()` + `SetOwnerLocator(...)`

3. **월드/타일/충돌**
   - `WorldSystem.LoadTileset(dev, L"tiles.png", tileW, tileH)`

- CSV 또는 코드로 타일 정의 주입: `WorldSystem.DefineTile(id, TileDef{ solid, oneway, src })`
- `WorldSystem.LoadMapCSV(L"map.csv")` → `WorldSystem.RebuildColliders()`
- 카메라 월드 경계: `cam.SetWorldRect(world.WorldRectPx())`

4. **CSV로 스테이지 요소 로드**

   - `LoadPlayerStartCSV("player.csv", out)`
   - `LoadMonstersCSV("monsters.csv", mons)` → `MonsterFactory::Create(...)`로 스폰
   - `LoadTileDefsCSV("tiles.csv", defs)` → 규칙에 따라 `DefineTile` 반영

5. **플레이어/몬스터/투사체 구성**

   - `PlayerFSM.Init(&player.Body(), &world.Collision(), player.Animator(), cfg)`
   - `MonsterFactory::RegisterDefaults()` / **`ProjectileFactory::RegisterDefaults()`**

6. **카메라**

   - `cam.SetScreenSize(w, h)` → `cam.SetPixelSnap(true)` → 스폰 직후 `cam.SnapImmediate()`
   - 매 프레임: `cam.SetLookAt(player.Center()); cam.Update(dt)`

7. **메인 루프**

   ```cpp
   time.TickFrame();
   input.BeginFrame();

   // 스파이럴 방지 (예: 한 프레임 최대 5회)
   int steps = 0; const int MAX_STEPS = 5;
   while (time.ShouldFixedUpdate() && steps++ < MAX_STEPS) {
       FixedUpdate(time.FixedDelta()); // FSM/물리/충돌 등
       time.ConsumeFixedStep();
   }

   RenderFrame();
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

# PlaySession (런타임 오케스트라)

**역할**: 한 스테이지의 런타임 전 과정을 오케스트레이션한다.
타일/충돌/카메라/플레이어 FSM/몬스터/투사체/히트볼륨/문(도어) 전환/페이드/HUD/디버그 드로우까지 한 곳에서 관리한다.
구현은 유지보수성 향상을 위해 다음과 같이 분할되어 있다.

- `PlaySession.Core.cpp` – 수명/초기화/FixedUpdate/HUD 페이드 API
- `PlaySession.Stage.cpp` – 스테이지 로드(타일/정의/배경/PlayerStart/몬스터/문) & 카메라 경계
- `PlaySession.Combat.cpp` – 팩토리 등록/전투 시스템 초기화/이벤트 처리/히트 적용
- `PlaySession.Render.cpp` – 배경/월드(타일+플레이어+몬스터)/디버그/HUD/페이드 렌더
- `PlaySession.Door.cpp` – 문 오버랩 검사/전환 상태머신(FadeOut→Load→FadeIn)

> 설계 원칙: GameApp은 **루프·입력·렌더러 수명**에 집중하고, 런타임은 PlaySession이 전담한다.

---

## 공개 API

```cpp
namespace game {
class PlaySession {
public:
    struct CreateDesc {
        engine::IRenderer*        renderer = nullptr;   // 백버퍼 크기 질의
        engine::D3D11SpriteBatch* batch    = nullptr;
        engine::D3D11DebugDraw*   debug    = nullptr;
        engine::DWriteTextHUD*    textHUD  = nullptr;
        engine::Scene*            scene    = nullptr;   // Player 스폰에 사용
        RECT rcClient{};                                 // 초기 클라이언트 영역
    };

    ~PlaySession();

    // 수명/초기화
    void Initialize(const CreateDesc& d);        // 텍스처 로드, Player 스폰/애니 셋업, 팩토리 1회 등록
    void OnResize(int sw, int sh);

    // 스테이지 로딩/리로드
    bool LoadStage(const char* jsonPath);        // StageDesc(JSON) + CSV들 로드
    bool ReloadStage();                          // 현재 jsonPath로 재로드

    // 고정 업데이트(게임 로직 메인)
    void FixedUpdate(double fixedDt, const engine::Input& input);

    // 렌더
    void RenderParallaxBG(int ox, int oy, int sw, int sh);
    void RenderWorld(int ox, int oy, int sw, int sh);                   // 타일 → 플레이어 → 몬스터
    void RenderDebugGridAndColliders(int ox, int oy, int sw, int sh,
                                     bool drawEnabled);                 // 그리드/충돌/플레이어/몬스터/투사체/히트볼륨/문
    void RenderHUD(int fps, double fixedDt);
    void RenderOverlayFade(int sw, int sh);                             // 페이드 오버레이

    // 카메라/월드 헬퍼
    std::pair<int,int>          CameraOffsetInt() const;
    engine::Camera&             Camera();
    const engine::WorldSystem&  World() const;
    engine::WorldSystem&        World();
    RECT                        WorldRectPx() const;
    int                         PlayerFacing() const;
    game::Player*               Player() const;

    // 이벤트 외부 방출(필요 시)
    void DrainPlayerEvents(std::vector<game::PlayerEvent>& out);

    // 페이드/전환
    void StartFadeIn (float seconds, uint32_t rgb = 0xFFFFFFu);
    void StartFadeOut(float seconds, uint32_t rgb = 0xFFFFFFu);
    bool IsFading() const;
    void StartTransitionTo(const std::string& target,
                           float fadeOutSec=0.25f, float fadeInSec=0.20f);
};
} // namespace game
```

---

## 라이프사이클 & 처리 순서

### 1) Initialize

- 렌더 핸들/Scene 보관, Player 스폰 및 FSM 초기화, 텍스처 로드(`player.png`, `enemies.png`, `white 1×1`), 플레이어 애니 CSV 로드/`Idle` 재생.
- **팩토리 등록 1회 선행**: `MonsterFactory::RegisterDefaults()`, `ProjectileFactory::RegisterDefaults()`, `HitVolumeFactory::RegisterDefaults()`.

### 2) LoadStage(jsonPath)

- `StageDesc(JSON)` 로드: `tileset`, `tiledefs.csv`, `tilemap.csv`, `monsters.csv`, `player_start.csv`, `background.png`, `doors.csv`.
- `WorldSystem` 구성: 타일셋/타일정의 등록 → 맵 그리드 적용 → 충돌 콜라이더 재빌드.
- 배경 텍스처 로드(선택).
- `PlayerStartCSV`로 플레이어 위치/카메라 스냅.
- `MonsterCSV`로 몬스터 생성(팩토리): 비주얼 사이즈/스프라이트 src 지정, 스포너 콜백 연결

  - `SetProjectileSpawnerId`: 투사체 스폰(속도 벡터)
  - `SetHitVolumeSpawner`: 히트볼륨 스폰
  - `SetTargetQuery`: 플레이어 센터 질의

- `DoorCSV` 로드: 문 AABB + target(stage.json).
- **전투 시스템 초기화**는 월드/콜라이더/플레이어 배치 **이후**에 호출.

### 3) FixedUpdate(fixedDt, input)

1. PlayerFSM `Step → DrainEvents → handlePlayerEvents`
2. 몬스터 업데이트
3. 전투: 타깃 구축 → `ProjectileSystem.Step` / `HitVolumeSystem.Step` → 히트 이벤트 드레인/적용
4. 플레이어 애니 업데이트, 카메라(플레이어 센터 추적) 업데이트
5. 지연 스폰 flush, 전환 FSM 업데이트, 페이드 t 진행

### 4) Render

- GameApp에서 스프라이트 배치 `Begin` 후 호출:

  1. `RenderParallaxBG`
  2. `RenderWorld` (타일 → 플레이어 → 몬스터)
  3. `RenderOverlayFade`

- 배치 `End` 이후: `RenderDebugGridAndColliders`(그리드/충돌/플레이어/몬스터/투사체/히트볼륨/문), `RenderHUD`.

---

## 이벤트 처리 매핑(핵심만)

| PlayerEvent    | 동작                                                          |
| -------------- | ------------------------------------------------------------- |
| `DoorInteract` | 문 AABB와 플레이어 AABB 오버랩 시 `StartTransitionTo(target)` |
| `InhaleVolume` | `HitVolume "InhaleField"` 스폰                                |
| `SpitStar`     | `Projectile "Star"` 방향성 사격                               |
| `AirPuffShot`  | `Projectile "AirPuff"` 방향성 사격                            |
| `AbilityFire`  | `Projectile "FirePellet"` 3연사(속도 벡터)                    |
| `AbilitySpark` | `HitVolume "SparkAura"` 스폰                                  |
| `AbilityBeam`  | `HitVolume "BeamSweep"` 스폰                                  |

---

## 전투 시스템 초기화 포인트

```cpp
void PlaySession::initCombatSystems() {
    m_projSys.Initialize(m_World.WorldRectPx(), &m_World.Collision());
    m_hitSys.Initialize();
    // OwnerLocator: 플레이어는 FSM의 Facing, 몬스터는 Bounds 중심 + Velocity.x로 방향 추정
    m_hitSys.SetOwnerLocator([this](int ownerId, engine::Vec2& pos, int& fac){
        if (m_Player && m_Player->Id()==ownerId) { pos=m_Player->Center(); fac=m_PlayerFSM.Facing(); return true; }
        for (auto& mon: m_Monsters) if (mon && mon->Id()==ownerId) {
            int x,y,w,h; mon->GetBounds(x,y,w,h);
            pos = { x + w*0.5f, y + h*0.5f };
            fac = (mon->Velocity().x >= 0.f) ? +1 : -1;
            return true;
        }
        return false;
    });
}
```

> `Monster`에 `Center()/Facing()`이 없다는 전제에서 **API 추가 없이** 동작하도록 설계.

---

## 문(도어)/전환

- `DoorCSV`(헤더 필수): `x,y,w,h,target` (월드 px)
- `checkDoorInteract()`에서 플레이어 AABB와 문 AABB `Overlap` 시 전환 시작.
- `StartTransitionTo(target, fadeOutSec, fadeInSec)` → `updateTransition()`에서
  `FadeOut 완료 → LoadStage(target) → FadeIn` 상태머신 수행.
- `RenderOverlayFade`: `white 1×1` 텍스처를 화면 전체로 `0xAARRGGBB` 색으로 라스터.

---

## 디버그 드로우

- 그리드/월드 콜라이더: `CollisionSystem::DebugDraw`
- 플레이어 AABB(녹색), 문 AABB(하늘색)
- 몬스터: `Monster::RenderDebug(dbg, ox, oy)` (AABB+HP바)
- 투사체/히트볼륨: 각 시스템 `DebugDraw(dbg, ox, oy)` 호출

> GameApp에서 Batch `End` 후 호출하여 **스프라이트 위층**에 라인/박스를 그림.

---

## HUD

- `RenderHUD(int fps, double fixedDt)`

  - FPS/Δt, 상태 이름(Move/Action/Overlay), FSM 디버그 스냅샷(속도/접지/타이머/최근 AABB 등),
    발밑 좌표/타일 좌표, 카메라 오프셋/룩앳, 몬스터 수, HP, 현재 능력, 액션 타이머, 스테이지 파일명.

> HUD 토글/옵션은 필요 시 `m_showHUD` 등으로 확장.

---

## 데이터 포맷 요약

- **StageDesc(JSON)**:
  `tileset`, `tiledefs`, `tilemap`, `monsters`, `player_start`, `background`, `doors` (경로 문자열)
- **PlayerStartCSV**: `x,y,dir`
- **MonsterCSV**: `type,x,y,dir,attack,move`

  - `type`: `WaddleDee|WaddleDoo|HotHead|Sparky` (대소문자 무시)
  - `dir`: `-1|0|+1` (초기 방향), `attack/move`: `0/1`

- **DoorCSV(헤더 필수)**: `x,y,w,h,target`

---

## GameApp 통합 예시

```cpp
// Init
m_Session = std::make_unique<game::PlaySession>();
m_Session->Initialize({ m_Renderer.get(), m_Batch.get(), m_Debug.get(), m_TextHUD.get(), &m_Scene, rc });
m_Session->LoadStage("assets/stages/stage01/stage.json");

// FixedUpdate
m_Session->FixedUpdate(m_Time.FixedDelta(), m_Input);

// RenderFrame (SpriteBatch Begin~End 사이)
m_Session->RenderParallaxBG(ox, oy, sw, sh);
m_Session->RenderWorld(ox, oy, sw, sh);
m_Session->RenderOverlayFade(sw, sh);

// Debug/HUD
m_Session->RenderDebugGridAndColliders(ox, oy, sw, sh, m_debugDrawEnabled);
m_Session->RenderHUD(m_Time.FPS(), m_Time.FixedDelta());

// F5 핫리로드
if (m_Input.ActionPressed("Reload")) m_Session->ReloadStage();
```

---

## 주의/팁

- **팩토리 등록은 `Initialize()`에서 한 번만.**
  스테이지 로드보다 먼저 등록돼 있어야 몬스터 스폰이 정상 동작.
- **전투 시스템 초기화는 LoadStage 끝에서.**
  월드 경계/충돌/플레이어 배치가 끝난 후 초기화해야 정확한 WorldRect/타깃 구성이 가능.
- 몬스터 방향 추정은 `Velocity.x` 기준(+0 포함은 +1) — 정지 시 바라보기 유지가 필요하면 Monster 내부 상태로 확장.
- 스프라이트 그리기는 **발 밑 정렬**(AABB 하단 기준) + `VisualSize` 사용, 플레이어는 Animator의 `CurrentSrc()` 사용.
- Batch `Begin/End` 범위 안에서만 Draw 호출, 디버그 드로우는 그 이후 호출.

---

---

## 2) 게임 월드 구성

### 타일/맵 시스템

#### `engine::TileSet`

- **역할**: 타일 아틀라스 텍스처 보관(셀 크기) + id→`TileDef`(solid/oneway/src) 사전 + 월드 타일 크기 보관.
- **API**
  - `bool LoadAtlas(ID3D11Device*, const wchar_t* png, int cellW, int cellH)`
  - `void SetWorldTileSize(int tileW, int tileH)`
  - `void Define(int id, const TileDef& def)` / `const TileDef* Get(int id) const`
  - `bool TrySrcFromIndex(int idx, RECT* out) const` // 아틀라스 셀 그리드 기준(텍스처 픽셀)
  - 접근자: `Atlas()`, `CellW/H()`, `TileW/H()`, `Cols()`, `Rows()`

#### `engine::TileMap`

- **역할**: 맵 ID 그리드 보관/주입, SOLID 병합·ONEWAY 개별 등록으로 충돌 빌드, 가시 타일만 렌더.
- **API**
  - `bool LoadFromMemory(int w, int h, const int* ids)` // CSV 파싱은 게임 레이어 담당
  - `int  BuildSolidColliders(physics::CollisionSystem&, const TileSet&) const`
  - `void Render(D3D11SpriteBatch&, const TileSet&, int camOffX, int camOffY, int screenW, int screenH) const`
  - `void RenderScaled(D3D11SpriteBatch&, const TileSet&, int camOffX, int camOffY, int screenW, int screenH, int scale) const`
  - 접근자: `W()`, `H()`, `At(x,y)`
- **좌표계**
  - 소스(`RECT src`): **텍스처 픽셀**(셀 크기 `TileSet.CellW/H`)
  - 배치/컬링: **월드 픽셀**(타일 크기 `TileSet.TileW/H`)
  - 카메라 오프셋 `camOffX/Y`: **월드 픽셀**

#### `engine::WorldSystem`

- **역할**: `TileSet` + `TileMap` + `CollisionSystem` 오케스트레이션(로드/정의 주입/충돌 빌드/렌더).
- **API**
  - `bool LoadTileset(ID3D11Device*, const std::wstring& png, int cellW, int cellH)`
  - `void SetWorldTileSize(int tileW, int tileH)`
  - `bool SetMapFromMemory(int w, int h, const int* ids)`
  - `void DefineTile(int id, const TileDef& def)`
  - `void RebuildColliders()`
  - `void RenderVisible(D3D11SpriteBatch&, int ox, int oy, int screenW, int screenH) const`
  - `void RenderVisibleScaled(D3D11SpriteBatch&, int ox, int oy, int screenW, int screenH, int scale) const`
  - `RECT WorldRectPx() const`
  - 접근자: `Tiles()`, `Map()`, `Collision()`, `TileW/H()`, `MapW/H()`

#### `game::StageCSV` (게임 레이어)

- **역할**: 모든 CSV 파싱 담당(엔진은 파일 포맷 비의존).
- **API**
  - `bool LoadTileMapCSV(const char* path, int& outW, int& outH, std::vector<int>& outIds)`
  - `bool LoadTileDefsCSV(const char* path, std::vector<TileDefCSV>& out)` // `TileDefCSV{ id, solid, oneway, gx, gy }`
  - `bool LoadPlayerStartCSV(const char* path, PlayerStartCSV& out)`
  - `bool LoadMonstersCSV(const char* path, std::vector<MonsterCSV>& out)`

### 스케일 합의

- **셀 크기(Cell)**: 아틀라스 소스 격자(예: 16×16)
- **월드 타일 크기(World)**: 화면 배치 크기(예: 64×64)
- 기본 렌더는 1×(추가 스케일 없음). 필요 시 `RenderScaled(..., scale)` 사용.

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

### 플레이어 FSM (병렬 트랙: Movement/Action/Overlay)

Kirby-like 특성에 맞춘 **병렬 FSM(3-Track)**:

- **Movement (M)**: `Idle, Walk, Run, Crouch, Slide, Jump, Fall, Inflated, Ladder`
- **Action (A)**: `Neutral, Inhale, MouthFull, SpitObject, AirPuff, AbilityAtk`
- **Overlay (Z)**: `None, Damaged, Dead, DoorEnter, Dance, GameOver`

핵심 포인트:

1. **입력 매핑**

   - **Z**: Jump / (공중) Inflated 진입 / (Inflated) 날개짓 / (Crouch) Sliding Kick
   - **X**: Inhale / (MouthFull) Spit / (Inflated) AirPuff / (Ability) AbilityAtk / (Crouch) Slide
   - **←/→**: 이동 (Walk→동방향 재입력 시 Run 가속 기동)
   - **↑**: 문 입장 (DoorEnter)
   - **↓**: Crouch / (MouthFull) Swallow

2. **점프 락 & Inflated 진입**

   - `IntegrateAndCollide()`의 버퍼 점프 성공 시 `m_jumpLockT = cfg.jumpLockMs`
   - `M_Jump::Update()`에서 **락이 끝난 뒤** 공중 `Z`를 새로 **Pressed**하면 `Inflated`로 전이

3. **Inflated 유지 규칙**

   - 공중 `Z`로 진입 후 **키를 떼도 유지**
   - **데미지**를 받거나 **X로 AirPuff 발사** 시 **즉시 종료**
   - 소프트폴: 낙하속도 캡(예: `vy <= 80.f`), `Z` 탭 시 날개짓 상승(짧은 `vy = -240.f` 등)

4. **Spit/AirPuff 단발 이벤트**

   - `A_SpitObject`/`A_AirPuff`는 **상태 멤버 `fired`**로 **첫 Update에서만 이벤트** 발생
   - `m_spitLockT`은 **`Step()`에서만 감소**
   - 락 종료 시 `A_Neutral`로 복귀 → 이동 봉인 해제

5. **이벤트 설계 (FSM→Game)**

   ```cpp
   struct PlayerEvent {
     enum Type { InhaleVolume, SpitStar, AirPuffShot, SwallowAbility, AbilityGained } type;
     RECT rect{};     // InhaleVolume: 흡입 범위
     int  facing{+1}; // +1/-1
     Ability ability{ Ability::None };
   };
   ```

6. **디버그 스냅샷**

   - `DebugInfo`에 `inhaleActive`, `inhaleRect` 추가 → `RenderDebug()`에서 시각화

7. **전이 가드 예시**
   ```cpp
   bool CanAct(AState from, AState to, const Ctx&) const {
     if (from==to) return false;
     if (m_zState != ZState::None) return false;
     if (from==AState::SpitObject && m_spitLockT > 0.f) return false; // 재진입 방지
     return true;
   }
   ```

### 몬스터

- `game::Monster` (베이스): `PhysicsBody`, `Animator`, `Health`, `StepPhysics` 공통, 히트/넉백/무적 처리.
  - 오버라이드 지점: `TickAI(fixedDt, input)`
  - 유틸: `HasGroundAhead(dir)`, `SetProjectileSpawner`, `SetTargetQuery`
- 샘플
  - `WaddleDee`: 단순 좌우 순찰(벽/절벽에서 방향 전환 옵션).
  - `WaddleDoo`: 순찰 + 사격(감지/윈드업/쿨다운/탄속 파라미터).

### 프로젝타일 시스템 (Projectile + Factory + System)

**목표**: FSM은 "행동 결정/이벤트"만, **스폰/파라미터/자원**은 **팩토리/데이터**에 위임.

- `Projectile`

  ```cpp
  struct Projectile::Cfg {
    float width=8, height=8, speed=480, ttl=1.5;
    bool  dieOnAnyWorldHit=true;
    // 물리 오버라이드
    float gravity=0, frictionAir=0, frictionGround=0, termVel=99999;
    bool  ignoreOneWay=true;
  };
  // 생성자에서 Cfg → PhysicsBody::Params() 반영
  ```

- `ProjectileFactory`

  ```cpp
  struct ProjDef {
    float width=8, height=8, speed=480, ttl=1.5;
    float gravity=0, frictionAir=0, frictionGround=0, termVel=99999;
    bool  dieOnAnyWorldHit=true, ignoreOneWay=true;
    int   damage=1; engine::Vec2 knockback{0,0};
  };
  // Create()에서 ProjDef → Projectile::Cfg 복사 후 인스턴스 생성
  // RegisterDefaults(): "Star", "AirPuff" 등 직선탄 기본값 (중력=0, 원웨이 무시)
  ```

- `ProjectileSystem`

  - **책임**: 스폰/업데이트/월드충돌(투사체 내부 위임), **엔티티 오버랩 판정**, 히트 이벤트 방출, 수명 관리.
  - **비책임**: 렌더/자원 로딩, 피해 적용(이벤트 소비자는 게임 레이어).
  - **핵심 타입**
    ```cpp
    struct CombatTarget { int id; RECT aabb; bool alive; bool isPlayer; };  // 공통 타깃 표현
    struct HitEvent {
      int targetId, projectileId; ProjOwner owner;
      ProjPayload payload; engine::Vec2 incomingDir; RECT projectileAabb;
    };
    struct SpawnDesc {
      std::string archetype; ProjOwner owner;
      engine::Vec2 pos, dirOrVel; bool treatAsDirection=true;
    };
    ```
  - **사용 예**
    ```cpp
    // 발사
    ProjectileSystem::SpawnDesc sd{ "Star", ProjOwner::Player, mouthPos, {+1,0}, true };
    m_projSys.Spawn(sd);
    // 틱
    std::vector<CombatTarget> targets = GatherTargets();  // 플레이어+몬스터
    m_projSys.Step(fixedDt, targets);
    std::vector<ProjectileSystem::HitEvent> phits;
    m_projSys.DrainHitEvents(phits);
    for (auto& e: phits) ApplyDamageById(e.targetId, {e.payload.damage, e.payload.knockback});
    ```

- **주의/가이드**
  - **팀 필터**: `owner`로 구분(플레이어 탄은 적만, 적 탄은 플레이어만).
  - **Pierce/바운스**: `ProjDef` 확장(`pierceCount` 등) → System이 히트 후 `Kill()` 대신 카운터 감소.
  - **등록 기본값**: 빔/스파크류는 **투사체가 아니라 히트볼륨**으로 구현. ProjectileFactory 기본 등록에서 `Beam`/`SparkBolt(임시)`는 제거.

```cpp
// (구) GameApp에서 직접 new/Fire/벡터 관리 예시는 폐기.
// (신) ProjectileSystem 사용으로 스폰/업데이트/피격이 한 곳에 모인다.
```

### 히트 볼륨 시스템 (HitVolume + Factory + System)

**목표**: 근접/오라/스윕 류 공격을 **타일 충돌 없이** 효율적으로 처리. (Spark/Beam)

- `HitVolume`

  - **모양**: `Box / Circle / Capsule`
  - **행동**: `Attached / AreaPulse / MeleeArc`
  - **설정(Cfg)**: `ttl, armTime, perTargetOnce, tickIntervalMs, startDeg/endDeg/sweepDuration, len/thick, localOffset, followFacing, payload{damage,knockback}`
  - **자가 히트 방지**: 소유자(`ownerId`)는 내부에서 제외.

- `HitVolumeFactory`

  - **기본 프리셋**:
    - `SparkAura`: `Circle`, `ttl≈0.9`, `tickIntervalMs≈120`, `perTargetOnce=false`
    - `BeamSweep`: `Capsule + MeleeArc`, `perTargetOnce=true`, **스윕 튜널링 방지** 포함(아래 System 참조)

- `HitVolumeSystem`
  - **책임**: 스폰/업데이트/오너 추적(앵커·페이싱), **엔티티 오버랩**, **이벤트 방출**, 수명 관리, 디버그 드로우.
  - **이벤트**
    ```cpp
    struct HitEvent { int targetId, volumeId, ownerId; HitPayload payload; };
    ```
  - **스윕 안정화**: 캡슐(`Capsule`)의 경우 프레임 간 놓침 방지를 위해
    ① 현재 캡슐, ② 이전 프레임 캡슐, ③ 팁(prevB→currB) 스윕 캡슐 **모두 검사**.
  - **디버그 드로우**: 캡슐 굵기 시각화(중심선 + 좌/우 오프셋 라인 + 양 끝 반원 근사).
  - **사용 예**
    ```cpp
    // Spark
    m_hitSys.Spawn({ "SparkAura", playerId, facing, playerCenter });
    // Beam (손/입 위치 권장)
    m_hitSys.Spawn({ "BeamSweep", playerId, facing, handPos });
    // 틱
    std::vector<CombatTarget> targets = GatherTargets();
    m_hitSys.Step(fixedDt, targets);
    std::vector<HitVolumeSystem::HitEvent> hvHits;
    m_hitSys.DrainHitEvents(hvHits);
    for (auto& e: hvHits) ApplyDamageById(e.targetId, {e.payload.damage, e.payload.knockback});
    ```

---

- **GameApp 이벤트 처리 (FSM→팩토리→스폰)**

  ```cpp
  // Step() 직후
  std::vector<game::PlayerEvent> evs;
  m_PlayerFSM.DrainEvents(evs);

  auto mouthPos = [&](int w=8,int h=8){
    int px,py,pw,ph; m_Player->GetBounds(px,py,pw,ph);
    float x = (m_PlayerFSM.Facing()>0) ? float(px+pw+1) : float(px-1-w);
    float y = float(py + ph*0.5f - h*0.5f);
    return engine::Vec2{ x, y };
  };

  for (auto& e : evs) switch (e.type) {
    case game::PlayerEvent::SpitStar: {
      RECT wr = m_World.WorldRectPx();
      auto p = game::ProjectileFactory::Create("Star", wr, &m_World.Collision(), game::ProjOwner::Player);
      if (p) { auto pos = mouthPos(int(p->Cfg().width), int(p->Cfg().height));
               float s = p->Cfg().speed; // 또는 ProjDef 조회
               p->Fire(pos, { float(m_PlayerFSM.Facing()) * s, 0.f });
               m_Projectiles.push_back(std::move(p)); }
      break;
    }
    case game::PlayerEvent::AirPuffShot: {
      RECT wr = m_World.WorldRectPx();
      auto p = game::ProjectileFactory::Create("AirPuff", wr, &m_World.Collision(), game::ProjOwner::Player);
      if (p) { auto pos = mouthPos(int(p->Cfg().width), int(p->Cfg().height));
               float s = p->Cfg().speed;
               p->Fire(pos, { float(m_PlayerFSM.Facing()) * s, 0.f });
               m_Projectiles.push_back(std::move(p)); }
      break;
    }
    case game::PlayerEvent::InhaleVolume:
      // e.rect 범위 내 몬스터 흡입 처리
      break;
    default: break;
  }
  ```

- **렌더링**: 스프라이트 배치로 사각형 또는 텍스처 드로우.  
  원웨이 관통 필요 시 `ignoreOneWay=true` 권장.

### 애니메이션 & CSV 로더

- `Animator`:
  - `AddClip(name, AnimClip)`, `Play(name, reset)`, `Update(dt)`
  - **추가**: `Clear()`, `HasClip(name)`, `RemoveClip(name)`
- CSV 로더 예시 (`game::LoadAnimCSV`)
  - 레코드
    - `strip, name, sx,sy, fw,fh, count, dur, loop(0/1)`
    - `frame, name, sx,sy, w,h, dur, loop(0/1)`
  - 로드 성공 시 `Animator->Clear(); AddClip(...)` 등록
  - **GameApp::Init**에서 하드코딩 제거 → `LoadAnimCSV("assets/player.anim.csv", player.Animator())`

---

## 4) 데이터 로딩

### CSV 로더 (`StageCSV`)

- `LoadPlayerStartCSV(path, out)`
- `LoadMonstersCSV(path, out)` — 필드 미지정은 `-1`/기본값으로 처리.
- `LoadTileDefsCSV(path, out)` — `id/solid/oneway` 읽어옴. 아틀라스 src 매핑은 별도 규칙 필요.

---

## 구현/사용 상 주의점 요약

- **Projectile vs HitVolume 경계**

* - **원거리/타일과 상호작용** → Projectile
* - **근접/오라/스윕(타일 무시)** → HitVolume
    +- **자가 히트 금지**
* - HitVolumeSystem은 `ownerId`를 내부적으로 제외.
* - ProjectileSystem은 `owner`(팀) 기반으로 타깃 분리.
    +- **프레임 간 놓침 방지**
* - Beam(캡슐)은 이전/현재/팁 스윕 3중 검사로 안정화.
    +- **이벤트 소비**
* - 두 System 모두 “명중 이벤트”만 방출, **피해 적용은 게임 레이어**에서 일관 처리.
    +- **공통 타깃 타입**
* - `CombatTarget`(id, aabb, alive, isPlayer)을 두 System에서 공유하여 변환 비용 제거.
    +- **기본 등록**
* - ProjectileFactory: `Star/AirPuff/FirePellet` 등만. `Beam/SparkBolt` 제거.
* - HitVolumeFactory: `SparkAura/BeamSweep` 제공(필요 시 CSV/JSON로 외부화).

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

## 통합 사용 예시 (핵심 스니펫)

1. **PlayerFSM 초기화**

```cpp
m_PlayerFSM.Init(&m_Player->Body(), &m_World.Collision(),
                 m_Player->Animator(),
                 { .jumpSpeed=700.f, .coyoteMs=0.08f, .bufferMs=0.10f, .dropMs=0.20f });
```

2. **ProjectileFactory 등록**

```cpp
game::MonsterFactory::RegisterDefaults();
game::ProjectileFactory::RegisterDefaults(); // ✅ 반드시 호출
```

3. **GameApp에서 임시 발사 제거 → 이벤트 기반 처리**

- Attack 키 즉발 발사 코드 **삭제**
- `m_PlayerFSM.DrainEvents(...)` 후 `Create("Star"/"AirPuff")`로 스폰

4. **디버그: Inhale 박스 표시**

- `DebugInfo`에 `inhaleActive, inhaleRect` 추가
- `RenderDebug()`에서 `WorldRect()`/`WorldLine()`으로 시각화

---

## 디버그/HUD/툴링

- HUD에 FSM 스냅샷 출력
  - `VEL, grounded(raw/stable), timers(coyote/buffer/drop/groundHold)`
  - **ActState/MoveState/OverlayState**, **HP/iFrameT**, **Facing**
  - **Inflated** 시 `vy 캡`/날개짓 흔적 확인
- 디버그 드로우
  - 타일 그리드, SOLID/ONEWAY 콜라이더, 플레이어/몬스터 AABB
  - **흡입 범위(InhaleVolume)** 박스

---

## 구현/사용 상 주의점 요약

- **Spit/AirPuff 반복 생성 방지**

  - `A_SpitObject/A_AirPuff`에 `bool fired` 상태 멤버 → **첫 Update에서만** 이벤트 발행
  - `m_spitLockT`는 **Step()에서만** 감소 (상태 내에서 건드리지 않기)
  - `A_Neutral`에서 Spit 트리거는 **Pressed**로 (홀드 사용 금지)

- **Inflated 유지**

  - 자동 복귀 없음. **데미지** 또는 **AirPuff 발사**로만 종료
  - 소프트폴/날개짓 적용

- **중력 0 직선탄**

  - `ProjectileFactory`의 `ProjDef` → `Projectile::Cfg`로 **물리 오버라이드 복사**
  - `Projectile` 생성자에서 `Cfg` 값을 **PhysicsBody::Params()`에 반영**
  - (필요) `ignoreOneWay=true`로 원웨이 관통

- **GameApp 정리**

  - 플레이어 물리 파라미터/타이머는 **FSM 단일 소스** (GameApp에서 제거)
  - `m_facing` 대신 **`m_PlayerFSM.Facing()`** 사용
  - 투사체 렌더링 루프 추가

- **Animator**
  - `Animator::Clear()` 추가 후 CSV 로딩 시 재등록
  - 하드코딩 애니 초기화 제거

---

## 업데이트 변경 이력

- **FSM**: 병렬 트랙(M/A/Z) 도입, Jump-Lock, Inflated 유지/종료 규칙, Inhale 디버그 박스
- **Action**: Spit/AirPuff **단발 이벤트화**, 락 타이머 중앙 감소
- **Projectile**: Factory/Def/Cfg 구조, **물리 오버라이드(중력/마찰/원웨이)** 지원
- **GameApp**: 임시 발사 제거, 이벤트 기반 스폰, 디버그/HUD 갱신
- **Animator**: `Clear/RemoveClip/HasClip` 추가, **CSV 로딩** 경로 확보

---
