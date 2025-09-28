#include "gamePCH.h"
#include "CStageScene.h"
#include "CSceneMgr.h"
#include "CKirby.h"
#include "CMonsterFactory.h"
#include "CBossTrigger.h"
#include "CCamera.h"

// --- 임시 CSV 로더 자리 — 실제 파서는 나중에 붙이자 ---
static bool LoadCSVIntGrid(const std::wstring& path, int w, int h, std::vector<int>& out)
{
    (void)path;
    out.assign(w * h, 0);
    return true;
}

CStageScene::CStageScene() { }
CStageScene::~CStageScene() { clearBuilt(); }

bool CStageScene::LoadStageFromFile(const std::wstring& path)
{
    (void)path;
    // TODO(json): StageDesc로 파싱해서 LoadStage(desc) 호출
    return false;
}

void CStageScene::LoadStage(const StageDesc& desc)
{
    clearBuilt();
    m_desc = desc;

    // 1) 타일 팔레트 — 실제로는 desc.tilesetJson에서 로드
    m_tileset.Clear();
    m_tileset.Set(0, TileDef{ L"", L"", Vec2(0,0), TILE_COLLISION::NONE });
    m_tileset.Set(1, TileDef{ L"content\\anim\\tiles.json", L"ground_block",  Vec2(0,0), TILE_COLLISION::SOLID });
    m_tileset.Set(2, TileDef{ L"content\\anim\\tiles.json", L"platform_wood", Vec2(0,0), TILE_COLLISION::ONEWAY_TOP });
    m_tileset.Set(3, TileDef{ L"content\\anim\\tiles.json", L"spike_idle",    Vec2(0,1), TILE_COLLISION::HAZARD, 6.f, 1, false });

    // 2) 타일맵 준비/빌드
    m_tilemap = std::make_unique<CTileMap>(&m_tileset, this, desc.originLT, desc.tileSize, /*w*/64, /*h*/32);
    m_tileFactory = std::make_unique<CTileFactory>(&m_tileset, this);

    // TODO(csv): desc.tilemapCsv 로드해서 m_tilemap->Set 채우기
    m_tilemap->Fill(0);
    for (int x = 0; x < 64; ++x) m_tilemap->Set(x, 20, 1);
    for (int x = 10; x < 20; ++x) m_tilemap->Set(x, 14, 2);
    for (int x = 25; x < 28; ++x) m_tilemap->Set(x, 19, 3);

    m_tilemap->BuildToScene();

    // 3) 스폰
    buildSpawns();

    // 4) 카메라 시작점
    CCamera::GetInst()->SetLookAtImmediate(desc.kirbySpawn);
}

void CStageScene::Enter() { CScene::Enter(); }
void CStageScene::Exit() { clearBuilt(); CScene::Exit(); }
void CStageScene::Update() { CScene::Update(); }

void CStageScene::clearBuilt()
{
    if (m_tilemap) { m_tilemap->ClearSceneObjects(); m_tilemap.reset(); }
    m_tileFactory.reset();

    m_kirby = nullptr;
    m_monFactory.reset();
}

void CStageScene::buildSpawns()
{
    // Kirby
    if (!m_kirby) {
        m_kirby = new CKirby();
        m_kirby->SetPos(m_desc.kirbySpawn);
        AddObject(m_kirby, GROUP_TYPE::PLAYER );
    }
    else {
        m_kirby->SetPos(m_desc.kirbySpawn);
    }

    // 몬스터
    if (!m_monFactory) m_monFactory = std::make_unique<CMonsterFactory>(this);
    for (const auto& s : m_desc.monsters) {
        if (auto* m = m_monFactory->Create(s.type, s.pos))  // 너의 팩토리 시그니처에 맞춰 사용
            AddObject( m, GROUP_TYPE::MONSTER);
    }

    // 보스 트리거
    for (const auto& t : m_desc.bossTriggers) {
        auto* trig = new CBossTrigger(t.worldLT, t.size, t.lockPos);
        AddObject( trig, GROUP_TYPE::TRIGGER);
    }
}
