#pragma once
#include "CTileSet.h"
#include "CScene.h"

class CTileFactory
{
public:
    CTileFactory(const CTileSet* tileset, CScene* scene)
        : m_tileset(tileset), m_scene(scene) {}

    void SetTileSet(const CTileSet* ts) { m_tileset = ts; }
    void SetScene(CScene* s) { m_scene = s; }

    // 타일 생성 + 씬 등록 + InitOnce 보장
    // worldLT : 타일 좌상단(월드), tileSize : 픽셀 크기
    CTile* Create(int tileId, const Vec2& worldLT, const Vec2& tileSize,
        GROUP_TYPE group = GROUP_TYPE::TERRAIN)
    {
        assert(m_tileset && m_scene);
        CTile* tile = m_tileset->CreateTile(tileId, worldLT, tileSize);
        m_scene->AddObject(tile ,group );     // 내부에서 InitOnce() 호출되게 앞서 패치했음
        return tile;
    }

    // 기존 타일에 팔레트 정의 적용 (에디터에서 교체)
    bool Apply(CTile* tile, int tileId) const
    {
        assert(m_tileset);
        return m_tileset->ApplyToTile(tile, tileId);
    }

private:
    const CTileSet* m_tileset{ nullptr };
    CScene* m_scene{ nullptr };
};
