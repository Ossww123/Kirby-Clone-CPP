#include "gamePCH.h"
#include "CTileSet.h"

void CTileSet::Set(int tileId, const TileDef& def)
{
    m_defs[tileId] = def;
}

bool CTileSet::Remove(int tileId)
{
    return m_defs.erase(tileId) > 0;
}

void CTileSet::Clear()
{
    m_defs.clear();
}

bool CTileSet::Has(int tileId) const
{
    return m_defs.find(tileId) != m_defs.end();
}

const TileDef* CTileSet::Get(int tileId) const
{
    auto it = m_defs.find(tileId);
    return (it == m_defs.end()) ? nullptr : &it->second;
}

bool CTileSet::ApplyToTile(CTile* pTile, int tileId) const
{
    if (!pTile) return false;
    const TileDef* def = Get(tileId);
    if (!def) return false;

    // 시각
    pTile->SetVisual(def->animFile, def->animName, def->visualOffset);

    // 충돌/옵션
    pTile->SetCollisionType(def->collision);
    pTile->SetOneWayThicknessPx(def->oneWayThicknessPx);
    pTile->SetDamage(def->damage);
    pTile->SetBlocksProjectiles(def->blocksProjectiles);

    return true;
}

CTile* CTileSet::CreateTile(int tileId, const Vec2& worldLT, const Vec2& tileSize) const
{
    const TileDef* def = Get(tileId);
    // 정의가 없으면 안전하게 빈 타일 생성
    const TILE_COLLISION coll = def ? def->collision : TILE_COLLISION::NONE;

    auto* tile = new CTile(worldLT, tileSize, coll);
    if (def)
    {
        tile->SetVisual(def->animFile, def->animName, def->visualOffset);
        tile->SetOneWayThicknessPx(def->oneWayThicknessPx);
        tile->SetDamage(def->damage);
        tile->SetBlocksProjectiles(def->blocksProjectiles);
    }
    return tile;
}
