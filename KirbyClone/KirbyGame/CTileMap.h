#pragma once
#include <vector>
#include <utility>
#include "CTileSet.h"   // TileDef, CTileSet, CTile
#include "CScene.h"

class CTileMap
{
public:
    CTileMap(const CTileSet* tileset,
        CScene* scene,
        const Vec2& originLT,
        const Vec2& tileSize,
        int width, int height,
        GROUP_TYPE visualGroup = GROUP_TYPE::TERRAIN,
        GROUP_TYPE solidGroup = GROUP_TYPE::TERRAIN);

    ~CTileMap();

    // ----- 그리드 편집 -----
    void Resize(int width, int height);
    void Set(int x, int y, int tileId);
    int  Get(int x, int y) const;

    // 한 번에 채우기(팔레트 ID)
    void Fill(int tileId);

    // ----- 빌드/정리 -----
    // 타일 비주얼 스폰 + 솔리드 머지 콜라이더 생성
    void BuildToScene();

    // 솔리드 머지 콜라이더만 재구축(맵 편집 시 자주 호출)
    void RebuildMergedColliders();

    // 맵에서 내가 만든 씬 오브젝트(타일/프록시) 전부 제거
    void ClearSceneObjects();

    // ----- 정보 -----
    int   GetWidth()  const { return m_w; }
    int   GetHeight() const { return m_h; }
    Vec2  GetTileSize()  const { return m_tileSize; }
    Vec2  GetOriginLT()  const { return m_originLT; }

    // (옵션) 시야 밖 스폰 억제 등에서 활용
    Vec2  CellWorldLT(int x, int y) const {
        return Vec2{ m_originLT.x + x * m_tileSize.x, m_originLT.y + y * m_tileSize.y };
    }

private:
    int   idx(int x, int y) const { return y * m_w + x; }
    bool  inRange(int x, int y) const { return (0 <= x && x < m_w && 0 <= y && y < m_h); }

    void  spawnVisualTiles();      // SOLID도 비주얼은 스폰(충돌은 제거)
    void  clearVisualTiles();      // 내가 만든 CTile들 제거
    void  clearSolidProxies();     // 내가 만든 솔리드 프록시 제거

    // 내부: 솔리드 머지(행 런→수직 병합, 최대 직사각형)
    void  buildSolidRects(std::vector<RECT>& out);

private:
    const CTileSet* m_tileset{ nullptr };
    CScene* m_scene{ nullptr };

    Vec2                    m_originLT;
    Vec2                    m_tileSize;
    int                     m_w{ 0 }, m_h{ 0 };
    std::vector<int>        m_ids;               // w*h

    GROUP_TYPE              m_visualGroup;
    GROUP_TYPE              m_solidGroup;

    std::vector<CTile*>     m_spawnedTiles;      // 씬에 추가한 CTile들
    std::vector<CObject*>   m_solidProxies;      // 씬에 추가한 대형 콜라이더 프록시
};
