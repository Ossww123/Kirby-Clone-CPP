#pragma once
#include <unordered_map>
#include <string>
#include "CTile.h"   // TILE_COLLISION, CTile

struct TileDef
{
    // 시각 리소스
    std::wstring animFile;              // 애니 JSON 경로
    std::wstring animName;              // 애니 이름(정적 1프레임도 애니로 통일)
    Vec2         visualOffset{ 0.f, 0.f };

    // 충돌/옵션
    TILE_COLLISION collision{ TILE_COLLISION::NONE };
    float          oneWayThicknessPx{ 6.f };   // ONEWAY_TOP일 때 상단 두께(px)
    int            damage{ 0 };                // HAZARD일 때 접촉 피해
    bool           blocksProjectiles{ true };  // 투사체 차단 여부
};

class CTileSet
{
public:
    CTileSet() = default;
    ~CTileSet() = default;

    // ── 정의 CRUD ───────────────────────────────────────────────
    void Set(int tileId, const TileDef& def);
    bool Remove(int tileId);
    void Clear();

    bool Has(int tileId) const;
    const TileDef* Get(int tileId) const;

    // ── 적용/생성 유틸 ───────────────────────────────────────────
    // 기존 타일에 정의 적용(시각/충돌 전부 세팅). 성공 시 true
    bool ApplyToTile(CTile* pTile, int tileId) const;

    // 타일 새로 만들어 반환(씬에 AddObject는 호출측에서)
    // worldLT : 좌상단 월드좌표, tileSize : 타일 픽셀 크기
    CTile* CreateTile(int tileId, const Vec2& worldLT, const Vec2& tileSize) const;

private:
    std::unordered_map<int, TileDef> m_defs;
};
