#include "pch.h"
#include "CTileMgr.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CTile.h"
#include "CCollider.h"

CTileMgr::CTileMgr()
{
}

CTileMgr::~CTileMgr()
{
    // 텍스처는 CResMgr에서 관리하므로 여기서는 맵만 클리어
    m_mapTileTexture.clear();
    m_mapTileInfo.clear();
}

// === 핵심 생명주기 함수 ===

void CTileMgr::init()
{
    CreateDefaultTileInfos();
    CreateDefaultTileTextures();
}

// === 타일 정보 관리 ===

void CTileMgr::RegisterTileInfo(const tTileInfo& _tileInfo)
{
    m_mapTileInfo.insert(make_pair(_tileInfo.eType, _tileInfo));
}

tTileInfo* CTileMgr::GetTileInfo(TILE_VISUAL_TYPE _eType) const
{
    auto iter = m_mapTileInfo.find(_eType);
    if (iter == m_mapTileInfo.end())
        return nullptr;

    return const_cast<tTileInfo*>(&iter->second);
}

Vec2 CTileMgr::GetTileDefaultSize(TILE_VISUAL_TYPE _eType) const
{
    tTileInfo* pInfo = GetTileInfo(_eType);
    if (pInfo)
        return pInfo->vDefaultSize;

    return Vec2(64.f, 64.f); // 기본값
}

void CTileMgr::CreateDefaultTileInfos()
{
    // 현재 지원하는 타일
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::TRANSPARENT_BLOCK, Vec2(64.f, 64.f), false, false, L""));

    // 향후 구현 예정 타일들 (예시)
    // RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::GRASS_PLATFORM, Vec2(64.f, 64.f), false, false, L"tiles\\grass_platform.bmp"));
    // RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::DIRT_BLOCK, Vec2(64.f, 64.f), false, false, L"tiles\\dirt_block.bmp"));
}

// === 타일 텍스처 관리 ===

CTexture* CTileMgr::LoadTileTexture(TILE_VISUAL_TYPE _eVisualType, const wstring& _strTexturePath)
{
    // 텍스처 경로가 없는 타일은 텍스처 없이 사용
    if (_strTexturePath.empty())
    {
        return nullptr;
    }

    // 이미 로드된 텍스처가 있는지 확인
    CTexture* pTex = FindTileTexture(_eVisualType);
    if (nullptr != pTex)
    {
        return pTex;
    }

    // 타일 타입별 고유 키 생성
    wstring strKey = L"Tile_";

    switch (_eVisualType)
    {
    case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK:
        strKey += L"TransparentBlock";
        break;
    case TILE_VISUAL_TYPE::BOSS_TRIGGER:
        strKey += L"TriggerBox";
        break;
        // 향후 구현 예정
        // case TILE_VISUAL_TYPE::GRASS_PLATFORM:
        //     strKey += L"GrassPlatform";
        //     break;
        // case TILE_VISUAL_TYPE::DIRT_BLOCK:
        //     strKey += L"DirtBlock";
        //     break;
    default:
        strKey += L"Unknown";
        break;
    }

    pTex = CResMgr::GetInst()->LoadTexture(strKey, _strTexturePath);

    if (nullptr == pTex)
    {
        // 텍스처 로드 실패시 로그 출력
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"타일 텍스처 로드 실패: %s", _strTexturePath.c_str());
        // MessageBox(nullptr, szBuffer, L"타일 텍스처 로드 실패", MB_OK);
        return nullptr;
    }

    // 맵에 추가
    m_mapTileTexture.insert(make_pair(_eVisualType, pTex));

    return pTex;
}

CTexture* CTileMgr::FindTileTexture(TILE_VISUAL_TYPE _eVisualType) const
{
    auto iter = m_mapTileTexture.find(_eVisualType);

    if (iter == m_mapTileTexture.end())
        return nullptr;

    return iter->second;
}

void CTileMgr::CreateDefaultTileTextures()
{
    // 향후 구현 예정: 등록된 타일 정보를 기반으로 텍스처 로드
    // 현재는 투명 블록만 지원하므로 텍스처 로딩 없음

    /*
    for (auto& pair : m_mapTileInfo)
    {
        const tTileInfo& info = pair.second;
        if (!info.strTexturePath.empty())
        {
            LoadTileTexture(info.eType, info.strTexturePath);
        }
    }
    */
}

// === 타일 시각 타입 유틸리티 ===

const wchar_t* CTileMgr::GetTileVisualName(TILE_VISUAL_TYPE _eType) const
{
    switch (_eType)
    {
    case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK:
        return L"Transparent Block";
    case TILE_VISUAL_TYPE::BOSS_TRIGGER:
        return L"Trigger Box";
    // 향후 구현 예정
    // case TILE_VISUAL_TYPE::GRASS_PLATFORM:
    //     return L"Grass Platform";
    // case TILE_VISUAL_TYPE::DIRT_BLOCK:
    //     return L"Dirt Block";
    default:
        return L"Unknown";
    }
}

vector<TILE_VISUAL_TYPE> CTileMgr::GetAvailableTileVisualTypes() const
{
    vector<TILE_VISUAL_TYPE> result;

    // 등록된 모든 타일 타입 반환
    for (auto& pair : m_mapTileInfo)
    {
        result.push_back(pair.first);
    }

    return result;
}

// === 타일 속성 설정 도우미 ===

void CTileMgr::SetupTileProperties(CTile* _pTile, TILE_VISUAL_TYPE _eVisualType)
{
    if (!_pTile)
        return;

    // 기본 크기 설정
    Vec2 vDefaultSize = GetTileDefaultSize(_eVisualType);
    _pTile->SetScale(vDefaultSize);

    // 시각적 타입 설정
    _pTile->SetVisualType(_eVisualType);

    // 향후 구현 예정: 텍스처 설정
    // CTexture* pTexture = FindTileTexture(_eVisualType);
    // if (pTexture)
    // {
    //     _pTile->SetTileTexture(pTexture);
    // }

    // 시각적 타입에 따른 기본 충돌 속성 설정
    switch (_eVisualType)
    {
    case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK:
        _pTile->SetCollisionType(COLLISION_TYPE::SOLID_GROUND);
        _pTile->SetSolid(true);
        _pTile->SetHarmful(false);
        _pTile->SetOneWay(false);
        break;

    case TILE_VISUAL_TYPE::BOSS_TRIGGER:
        _pTile->SetCollisionType(COLLISION_TYPE::TRIGGER);
        _pTile->SetSolid(false);    // 트리거는 통과 가능
        _pTile->SetHarmful(false);
        _pTile->SetOneWay(false);
        break;

        // 향후 구현 예정
        // case TILE_VISUAL_TYPE::GRASS_PLATFORM:
        //     _pTile->SetCollisionType(COLLISION_TYPE::PLATFORM);
        //     _pTile->SetSolid(true);
        //     _pTile->SetHarmful(false);
        //     _pTile->SetOneWay(true);
        //     break;

    default:
        // 기본값은 일반 땅 블록
        _pTile->SetCollisionType(COLLISION_TYPE::SOLID_GROUND);
        _pTile->SetSolid(true);
        _pTile->SetHarmful(false);
        _pTile->SetOneWay(false);
        break;
    }

    // 충돌체 설정
    bool needsCollider = _pTile->IsSolid() || _pTile->IsHarmful() || 
                        (_pTile->GetCollisionType() == COLLISION_TYPE::TRIGGER);
    
    if (needsCollider)
    {
        // 충돌이 필요한 타일이면 콜라이더 생성
        if (!_pTile->GetCollider())
        {
            _pTile->CreateCollider();
        }
        if (_pTile->GetCollider())
        {
            _pTile->GetCollider()->SetScale(vDefaultSize);
        }
    }

    // 기능적 타입 설정 - 이미 설정된 타일 타입이 있으면 유지
    OBJECT_TYPE currentTileType = _pTile->GetTileType();
    if (currentTileType == OBJECT_TYPE::END || currentTileType == (OBJECT_TYPE)0)
    {
        // 타일 타입이 설정되지 않은 경우에만 기본값으로 설정
        _pTile->SetTileType(OBJECT_TYPE::TILE_GROUND);
    }
    // 이미 올바른 타입(TILE_TRIGGER 등)이 설정되어 있으면 그대로 유지
}