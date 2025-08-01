#pragma once

class CTexture;
class CTile;

class CTileMgr
{
    SINGLE(CTileMgr);

private:
    map<TILE_VISUAL_TYPE, CTexture*> m_mapTileTexture;   // 타일 시각 타입별 텍스처
    map<TILE_VISUAL_TYPE, tTileInfo> m_mapTileInfo;      // 타일 정보

public:
    void init();

    // 타일 정보 관리
    void RegisterTileInfo(const tTileInfo& _tileInfo);
    tTileInfo* GetTileInfo(TILE_VISUAL_TYPE _eType);
    Vec2 GetTileDefaultSize(TILE_VISUAL_TYPE _eType);

    // 타일 텍스처 관리
    CTexture* LoadTileTexture(TILE_VISUAL_TYPE _eVisualType, const wstring& _strTexturePath);
    CTexture* FindTileTexture(TILE_VISUAL_TYPE _eVisualType);

    // 타일 시각 타입 관련 유틸리티
    const wchar_t* GetTileVisualName(TILE_VISUAL_TYPE _eType);
    vector<TILE_VISUAL_TYPE> GetAvailableTileVisualTypes();

    // 타일 속성 설정 도우미
    void SetupTileProperties(CTile* _pTile, TILE_VISUAL_TYPE _eVisualType);

private:
    void CreateDefaultTileInfos();       // 기본 타일 정보들 등록
    void CreateDefaultTileTextures();    // 기본 타일 텍스처들 로드
};