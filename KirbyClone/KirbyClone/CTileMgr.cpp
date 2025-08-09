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

void CTileMgr::init()
{
    CreateDefaultTileInfos();
    CreateDefaultTileTextures();
}

void CTileMgr::CreateDefaultTileInfos()
{
    /*
    // 기본 지형 타일들 (64x64 표준 크기)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::GRASS_PLATFORM, Vec2(64.f, 64.f), false, false, L"tiles\\grass_platform.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::DIRT_BLOCK, Vec2(64.f, 64.f), false, false, L"tiles\\dirt_block.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::STONE_BLOCK, Vec2(64.f, 64.f), false, false, L"tiles\\stone_block.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::GRASS_BLOCK, Vec2(64.f, 64.f), false, false, L"tiles\\grass_block.bmp"));

    // 큰 장식용 타일들 (나무는 더 클 수 있음)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::TREE, Vec2(96.f, 128.f), true, false, L"tiles\\tree.bmp"));

    // 작은 장식용 타일들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::FLOWER, Vec2(32.f, 32.f), true, false, L"tiles\\flower.bmp"));

    // 울타리 (가로로 긴 형태 가능)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::FENCE, Vec2(64.f, 48.f), false, false, L"tiles\\fence.bmp"));

    // 파이프 (세로로 긴 형태)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::PIPE, Vec2(64.f, 96.f), false, false, L"tiles\\pipe.bmp"));

    // 위험 요소들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::SPIKE, Vec2(64.f, 32.f), false, true, L"tiles\\spike.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::WATER, Vec2(64.f, 64.f), false, false, L"tiles\\water.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::LAVA, Vec2(64.f, 64.f), false, true, L"tiles\\lava.bmp"));

    // 플랫폼들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::MOVING_PLATFORM, Vec2(128.f, 32.f), false, false, L"tiles\\moving_platform.bmp"));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::BRIDGE, Vec2(96.f, 24.f), false, false, L"tiles\\bridge.bmp"));
    */

    // 기본 충돌체 타일들 (64x64 표준 크기) - 텍스처 경로는 빈 문자열로 설정
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::GRASS_PLATFORM, Vec2(64.f, 64.f), false, false, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::DIRT_BLOCK, Vec2(64.f, 64.f), false, false, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::STONE_BLOCK, Vec2(64.f, 64.f), false, false, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::GRASS_BLOCK, Vec2(64.f, 64.f), false, false, L""));

    // 큰 장식용 타일들 (나무는 더 클 수 있음) - 충돌체 전용이므로 크기만 유지
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::TREE, Vec2(96.f, 128.f), true, false, L""));

    // 작은 장식용 타일들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::FLOWER, Vec2(32.f, 32.f), true, false, L""));

    // 울타리 (가로로 긴 형태 가능)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::FENCE, Vec2(64.f, 48.f), false, false, L""));

    // 파이프 (세로로 긴 형태)
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::PIPE, Vec2(64.f, 96.f), false, false, L""));

    // 위험 요소들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::SPIKE, Vec2(64.f, 32.f), false, true, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::WATER, Vec2(64.f, 64.f), false, false, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::LAVA, Vec2(64.f, 64.f), false, true, L""));

    // 플랫폼들
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::MOVING_PLATFORM, Vec2(128.f, 32.f), false, false, L""));
    RegisterTileInfo(tTileInfo(TILE_VISUAL_TYPE::BRIDGE, Vec2(96.f, 24.f), false, false, L""));
}

void CTileMgr::CreateDefaultTileTextures()
{
    // 더 이상 타일 텍스처를 로드하지 않음 - 충돌체 전용 시스템
    // 이 함수는 호환성을 위해 유지하지만 아무것도 하지 않음

    // 모든 등록된 타일 정보를 기반으로 텍스처 로드 (비활성화됨)
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

void CTileMgr::RegisterTileInfo(const tTileInfo& _tileInfo)
{
    m_mapTileInfo.insert(make_pair(_tileInfo.eType, _tileInfo));
}

tTileInfo* CTileMgr::GetTileInfo(TILE_VISUAL_TYPE _eType)
{
    auto iter = m_mapTileInfo.find(_eType);
    if (iter == m_mapTileInfo.end())
        return nullptr;

    return &iter->second;
}

Vec2 CTileMgr::GetTileDefaultSize(TILE_VISUAL_TYPE _eType)
{
    tTileInfo* pInfo = GetTileInfo(_eType);
    if (pInfo)
        return pInfo->vDefaultSize;

    return Vec2(64.f, 64.f); // 기본값
}

CTexture* CTileMgr::LoadTileTexture(TILE_VISUAL_TYPE _eVisualType, const wstring& _strTexturePath)
{
    // 이미 로드된 텍스처가 있는지 확인
    CTexture* pTex = FindTileTexture(_eVisualType);
    if (nullptr != pTex)
    {
        return pTex;
    }

    // 더 이상 타일 텍스처를 사용하지 않으므로 로딩하지 않음
    return nullptr;

    // 타일 타입별 고유 키 생성 - 더 안전한 방법
    wstring strKey = L"Tile_";

    switch (_eVisualType)
    {
    case TILE_VISUAL_TYPE::GRASS_PLATFORM:   strKey += L"GrassPlatform"; break;
    case TILE_VISUAL_TYPE::DIRT_BLOCK:       strKey += L"DirtBlock"; break;
    case TILE_VISUAL_TYPE::STONE_BLOCK:      strKey += L"StoneBlock"; break;
    case TILE_VISUAL_TYPE::GRASS_BLOCK:      strKey += L"GrassBlock"; break;
    case TILE_VISUAL_TYPE::TREE:             strKey += L"Tree"; break;
    case TILE_VISUAL_TYPE::FLOWER:           strKey += L"Flower"; break;
    case TILE_VISUAL_TYPE::FENCE:            strKey += L"Fence"; break;
    case TILE_VISUAL_TYPE::PIPE:             strKey += L"Pipe"; break;
    case TILE_VISUAL_TYPE::SPIKE:            strKey += L"Spike"; break;
    case TILE_VISUAL_TYPE::WATER:            strKey += L"Water"; break;
    case TILE_VISUAL_TYPE::LAVA:             strKey += L"Lava"; break;
    case TILE_VISUAL_TYPE::MOVING_PLATFORM:  strKey += L"MovingPlatform"; break;
    case TILE_VISUAL_TYPE::BRIDGE:           strKey += L"Bridge"; break;
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

CTexture* CTileMgr::FindTileTexture(TILE_VISUAL_TYPE _eVisualType)
{
    auto iter = m_mapTileTexture.find(_eVisualType);

    if (iter == m_mapTileTexture.end())
        return nullptr;

    return iter->second;
}

const wchar_t* CTileMgr::GetTileVisualName(TILE_VISUAL_TYPE _eType)
{
    switch (_eType)
    {
    case TILE_VISUAL_TYPE::GRASS_PLATFORM:   return L"Grass Platform";
    case TILE_VISUAL_TYPE::DIRT_BLOCK:       return L"Dirt Block";
    case TILE_VISUAL_TYPE::STONE_BLOCK:      return L"Stone Block";
    case TILE_VISUAL_TYPE::GRASS_BLOCK:      return L"Grass Block";
    case TILE_VISUAL_TYPE::TREE:             return L"Tree";
    case TILE_VISUAL_TYPE::FLOWER:           return L"Flower";
    case TILE_VISUAL_TYPE::FENCE:            return L"Fence";
    case TILE_VISUAL_TYPE::PIPE:             return L"Pipe";
    case TILE_VISUAL_TYPE::SPIKE:            return L"Spike";
    case TILE_VISUAL_TYPE::LAVA:             return L"Lava";
    case TILE_VISUAL_TYPE::WATER:            return L"Water";
    case TILE_VISUAL_TYPE::MOVING_PLATFORM:  return L"Moving Platform";
    case TILE_VISUAL_TYPE::BRIDGE:           return L"Bridge";
    default:                                 return L"Unknown";
    }
}

vector<TILE_VISUAL_TYPE> CTileMgr::GetAvailableTileVisualTypes()
{
    vector<TILE_VISUAL_TYPE> result;

    // 등록된 모든 타일 타입 반환
    for (auto& pair : m_mapTileInfo)
    {
        result.push_back(pair.first);
    }

    return result;
}

void CTileMgr::SetupTileProperties(CTile* _pTile, TILE_VISUAL_TYPE _eVisualType)
{
    if (!_pTile)
        return;

    // 더 이상 텍스처 관련 처리는 하지 않음 - 충돌체 전용 시스템으로 변경

    // 기본 크기 설정 (모든 충돌체는 64x64)
    _pTile->SetScale(Vec2(64.f, 64.f));

    // 시각적 타입 설정 (호환성용으로만 유지)
    _pTile->SetVisualType(_eVisualType);

    // *** 텍스처 설정 제거 - 더 이상 사용하지 않음 ***
    // CTexture* pTexture = FindTileTexture(_eVisualType);
    // if (pTexture)
    // {
    //     _pTile->SetTileTexture(pTexture);
    // }

    // 시각적 타입에 따른 기본 충돌체 속성 설정
    switch (_eVisualType)
    {
    case TILE_VISUAL_TYPE::GRASS_PLATFORM:
        _pTile->SetCollisionType(COLLISION_TYPE::SOLID_GROUND);
        _pTile->SetDecorative(false);
        _pTile->SetHarmful(false);
        _pTile->SetSolid(true);
        break;

    case TILE_VISUAL_TYPE::SPIKE:
        _pTile->SetCollisionType(COLLISION_TYPE::SPIKE);
        _pTile->SetDecorative(false);
        _pTile->SetHarmful(true);
        _pTile->SetSolid(true);
        break;

    case TILE_VISUAL_TYPE::WATER:
        _pTile->SetCollisionType(COLLISION_TYPE::WATER);
        _pTile->SetDecorative(false);
        _pTile->SetHarmful(false);
        _pTile->SetSolid(false);
        break;

    case TILE_VISUAL_TYPE::LAVA:
        _pTile->SetCollisionType(COLLISION_TYPE::LAVA);
        _pTile->SetDecorative(false);
        _pTile->SetHarmful(true);
        _pTile->SetSolid(false);
        break;

    default:
        // 기본값은 일반 땅 블록
        _pTile->SetCollisionType(COLLISION_TYPE::SOLID_GROUND);
        _pTile->SetDecorative(false);
        _pTile->SetHarmful(false);
        _pTile->SetSolid(true);
        break;
    }

    // 충돌체 설정
    if (_pTile->IsSolid() || _pTile->IsHarmful())
    {
        // 충돌이 필요한 타입이면 콜라이더 생성
        if (!_pTile->GetCollider())
        {
            _pTile->CreateCollider();
        }
        if (_pTile->GetCollider())
        {
            _pTile->GetCollider()->SetScale(Vec2(64.f, 64.f));
        }
    }

    // 기능적 타일 타입 설정 (호환성용)
    OBJECT_TYPE eFunctionalType = OBJECT_TYPE::TILE_GROUND; // 기본값

    switch (_eVisualType)
    {
    case TILE_VISUAL_TYPE::SPIKE:
        eFunctionalType = OBJECT_TYPE::TILE_SPIKE;
        break;
    case TILE_VISUAL_TYPE::WATER:
        eFunctionalType = OBJECT_TYPE::TILE_WATER;
        break;
    case TILE_VISUAL_TYPE::LAVA:
        eFunctionalType = OBJECT_TYPE::TILE_LAVA;
        break;
    default:
        eFunctionalType = OBJECT_TYPE::TILE_GROUND;
        break;
    }

    _pTile->SetTileType(eFunctionalType);
}