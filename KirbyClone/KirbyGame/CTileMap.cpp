#include "gamePCH.h"
#include "CTileMap.h"
#include "CObject.h"
#include "CScene.h"
#include "CSceneMgr.h"
#include "CCollider.h"

// ===== 내부 전용: 솔리드 콜라이더 프록시 =====
class CTileSolidProxy : public CObject
{
public:
    CTileSolidProxy(const Vec2& worldCenter, const Vec2& worldSize)
    {
        SetPos(worldCenter);
        SetScale(Vec2(1.f, 1.f)); // 의미 없음. 디버그 사각형 크기일 뿐
        CreateCollider();
        GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
        GetCollider()->SetScale(worldSize);
    }
    void Update() override {} // 아무것도 안 함
};

// ===== CTileMap 구현 =====
CTileMap::CTileMap(const CTileSet* tileset, CScene* scene,
    const Vec2& originLT, const Vec2& tileSize,
    int width, int height,
    GROUP_TYPE visualGroup, GROUP_TYPE solidGroup)
    : m_tileset(tileset)
    , m_scene(scene)
    , m_originLT(originLT)
    , m_tileSize(tileSize)
    , m_w(width)
    , m_h(height)
    , m_visualGroup(visualGroup)
    , m_solidGroup(solidGroup)
{
    m_ids.assign(m_w * m_h, 0);
}

CTileMap::~CTileMap()
{
    ClearSceneObjects();
}

void CTileMap::Resize(int width, int height)
{
    ClearSceneObjects();
    m_w = width;  m_h = height;
    m_ids.assign(m_w * m_h, 0);
}

void CTileMap::Set(int x, int y, int tileId)
{
    if (!inRange(x, y)) return;
    m_ids[idx(x, y)] = tileId;
}

int CTileMap::Get(int x, int y) const
{
    if (!inRange(x, y)) return 0;
    return m_ids[idx(x, y)];
}

void CTileMap::Fill(int tileId)
{
    std::fill(m_ids.begin(), m_ids.end(), tileId);
}

void CTileMap::BuildToScene()
{
    // 1) 기존 생성분 제거
    ClearSceneObjects();

    // 2) 비주얼 타일들 스폰 (SOLID도 콜라이더는 비활성)
    spawnVisualTiles();

    // 3) SOLID 병합 콜라이더 생성
    RebuildMergedColliders();
}

void CTileMap::RebuildMergedColliders()
{
    clearSolidProxies();

    std::vector<RECT> rects;
    buildSolidRects(rects);

    // 병합 결과로 프록시 스폰
    for (const auto& rc : rects)
    {
        const int x0 = rc.left;
        const int y0 = rc.top;
        const int x1 = rc.right;  // exclusive
        const int y1 = rc.bottom; // exclusive

        const Vec2 worldLT = Vec2{
            m_originLT.x + x0 * m_tileSize.x,
            m_originLT.y + y0 * m_tileSize.y
        };
        const Vec2 size = Vec2{
            (x1 - x0) * m_tileSize.x,
            (y1 - y0) * m_tileSize.y
        };
        const Vec2 center = Vec2{ worldLT.x + size.x * 0.5f, worldLT.y + size.y * 0.5f };

        auto* proxy = new CTileSolidProxy(center, size);
        m_scene->AddObject(proxy, m_solidGroup);
        m_solidProxies.push_back(proxy);
    }
}

void CTileMap::ClearSceneObjects()
{
    clearVisualTiles();
    clearSolidProxies();
}

void CTileMap::spawnVisualTiles()
{
    m_spawnedTiles.reserve(m_w * m_h);

    for (int y = 0; y < m_h; ++y)
        for (int x = 0; x < m_w; ++x)
        {
            const int id = m_ids[idx(x, y)];
            const TileDef* def = m_tileset ? m_tileset->Get(id) : nullptr;

            // 정의가 없고 id==0 이면 빈칸 취급
            if (!def && id == 0) continue;

            const Vec2 cellLT = CellWorldLT(x, y);

            // 정의가 없으면(혹은 애니 없는 NONE) 스킵
            if (!def) continue;
            const bool hasVisual = (!def->animName.empty());
            const bool isEmpty = (def->collision == TILE_COLLISION::NONE) && !hasVisual;
            if (isEmpty) continue;

            // 타일 생성
            CTile* tile = m_tileset->CreateTile(id, cellLT, m_tileSize);

            // SOLID는 비주얼만 남기고 충돌은 프록시에게 맡김
            if (def->collision == TILE_COLLISION::SOLID)
                tile->SetCollisionType(TILE_COLLISION::NONE);

            m_scene->AddObject(tile, m_visualGroup);
            m_spawnedTiles.push_back(tile);
        }
}

void CTileMap::clearVisualTiles()
{
    if (!m_scene) { m_spawnedTiles.clear(); return; }

    for (auto* t : m_spawnedTiles)
    {
        // 씬에서 떼고 직접 delete (현재 씬 구현은 Dead 즉시 delete가 아님)
        m_scene->DetachObject(t);
        delete t;
    }
    m_spawnedTiles.clear();
}

void CTileMap::clearSolidProxies()
{
    if (!m_scene) { m_solidProxies.clear(); return; }

    for (auto* p : m_solidProxies)
    {
        m_scene->DetachObject(p);
        delete p;
    }
    m_solidProxies.clear();
}

// === SOLID 병합 ===
// 행 단위 런을 만들고, 바로 위 행의 동일 구간(x0..x1)을 가진 직사각형과 수직으로 병합.
// 결과는 타일 좌표계 기준의 [x0,x1), [y0,y1) 사각형 목록.
void CTileMap::buildSolidRects(std::vector<RECT>& out)
{
    out.clear();
    // key: (x0<<16)|x1  (폭이 65535 타일 미만이라는 가정, 보통 충분)
    auto keyOf = [](int x0, int x1)->int { return (x0 << 16) | (x1 & 0xFFFF); };

    // active: 이전 행에서 내려오던 직사각형들 (key -> RECT)
    std::unordered_map<int, RECT> active;

    for (int y = 0; y < m_h; ++y)
    {
        // 1) 현재 행의 수평 런 수집
        std::vector<std::pair<int, int>> runs; runs.reserve(m_w / 2 + 1);
        int x = 0;
        while (x < m_w)
        {
            // 이 칸이 SOLID인지 확인
            auto isSolid = [&](int gx, int gy)->bool {
                const int id = m_ids[idx(gx, gy)];
                const TileDef* def = m_tileset ? m_tileset->Get(id) : nullptr;
                return (def && def->collision == TILE_COLLISION::SOLID);
                };

            if (!isSolid(x, y)) { ++x; continue; }
            int x0 = x;
            while (x < m_w && isSolid(x, y)) ++x;
            int x1 = x;
            runs.emplace_back(x0, x1);
        }

        // 2) 수직 병합
        std::unordered_map<int, RECT> nextActive; nextActive.reserve(runs.size());
        for (auto [x0, x1] : runs)
        {
            const int k = keyOf(x0, x1);
            auto it = active.find(k);
            if (it != active.end())
            {
                // 바로 위와 가로폭 동일 → 아래로 확장
                RECT rc = it->second;
                rc.bottom = y + 1;
                nextActive[k] = rc;
            }
            else
            {
                // 새 직사각형 시작
                RECT rc;
                rc.left = x0; rc.right = x1;
                rc.top = y;  rc.bottom = y + 1;
                nextActive[k] = rc;
            }
        }

        // 3) 이번 행에서 이어지지 못한 active는 확정 출력
        for (auto& kv : active)
        {
            if (nextActive.find(kv.first) == nextActive.end())
                out.push_back(kv.second);
        }

        active.swap(nextActive);
    }

    // 마지막 행 누락분 flush
    for (auto& kv : active) out.push_back(kv.second);
}
