#pragma once
#include "CScene.h"
#include "CTileMap.h"
#include "CTileSet.h"
#include "CTileFactory.h"

// forward
class CKirby;
class CMonsterFactory;
class CBossTrigger;

struct SpawnDesc {
    std::wstring type;  // "WaddleDee", "HotHead" 등
    Vec2         pos;
};

struct BossTriggerDesc {
    Vec2 worldLT;
    Vec2 size;
    Vec2 lockPos; // 미지정이면 (0,0) → 트리거 중심을 사용
};

struct StageDesc {
    std::wstring name;

    // 타일세트/타일맵
    std::wstring tilesetJson;
    std::wstring tilemapCsv;
    Vec2         tileSize{ 16,16 };
    Vec2         originLT{ 0,0 };

    // 스폰
    Vec2                    kirbySpawn{ 64,64 };   // ← playerSpawn → kirbySpawn
    std::vector<SpawnDesc>  monsters;
    std::vector<BossTriggerDesc> bossTriggers;
};

class CStageScene : public CScene
{
public:
    CStageScene();
    ~CStageScene() override;

    bool LoadStageFromFile(const std::wstring& path); // TODO(parse)
    void LoadStage(const StageDesc& desc);

    void Enter() override;
    void Exit()  override;
    void Update() override;

    const StageDesc& GetDesc() const { return m_desc; }
    CKirby* GetKirby() const { return m_kirby; }   // ← CPlayer → CKirby

private:
    void clearBuilt();
    void buildTiles();
    void buildSpawns();

private:
    StageDesc                     m_desc;
    CTileSet                      m_tileset;
    std::unique_ptr<CTileMap>     m_tilemap;
    std::unique_ptr<CTileFactory> m_tileFactory;

    CKirby* m_kirby{ nullptr };       // ← CPlayer* → CKirby*
    std::unique_ptr<CMonsterFactory>   m_monFactory;
};
