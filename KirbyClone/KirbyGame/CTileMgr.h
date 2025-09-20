#pragma once

class CTexture;
class CTile;

class CTileMgr
{
    SINGLE(CTileMgr);

public:
    // === 핵심 생명주기 함수 ===
    void init();

public:
    // === 타일 정보 관리 ===
    void RegisterTileInfo(const tTileInfo& _tileInfo);
    tTileInfo* GetTileInfo(TILE_VISUAL_TYPE _eType) const;
    Vec2 GetTileDefaultSize(TILE_VISUAL_TYPE _eType) const;

private:
    void CreateDefaultTileInfos();

public:
    // === 타일 텍스처 관리 ===
    CTexture* LoadTileTexture(TILE_VISUAL_TYPE _eVisualType, const wstring& _strTexturePath);
    CTexture* FindTileTexture(TILE_VISUAL_TYPE _eVisualType) const;

private:
    void CreateDefaultTileTextures();

public:
    // === 타일 시각 타입 유틸리티 ===
    const wchar_t* GetTileVisualName(TILE_VISUAL_TYPE _eType) const;
    vector<TILE_VISUAL_TYPE> GetAvailableTileVisualTypes() const;

public:
    // === 타일 속성 설정 도우미 ===
    void SetupTileProperties(CTile* _pTile, TILE_VISUAL_TYPE _eVisualType);

private:
    // === 타일 데이터 ===
    map<TILE_VISUAL_TYPE, CTexture*>    m_mapTileTexture;   // 타일 시각 타입별 텍스처
    map<TILE_VISUAL_TYPE, tTileInfo>    m_mapTileInfo;      // 타일 정보
};