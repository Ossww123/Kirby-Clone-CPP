#include "pch.h"
#include "CObjectFactory.h"
#include "CTileMgr.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CCollider.h"
#include "CMonster.h"
#include "CItem.h"
#include "CTile.h"
#include "CSpecialObject.h"
#include "CDoor.h"

CObject* CObjectFactory::CreateObject(OBJECT_TYPE _eType, Vec2 _vPos)
{
    if (!IsValidObjectType(_eType))
        return nullptr;

    CObject* pObject = nullptr;

    // 객체 타입에 따라 적절한 생성 함수 호출
    switch (_eType)
    {
    case OBJECT_TYPE::PLAYER:
        pObject = CreatePlayer(_vPos);
        break;

        // 몬스터 타입들
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
    case OBJECT_TYPE::MONSTER_GORDOS:
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        pObject = CreateMonster(_eType, _vPos);
        break;

        // 아이템 타입들
    case OBJECT_TYPE::ITEM_STAR:
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:
    case OBJECT_TYPE::ITEM_1UP:
    case OBJECT_TYPE::ITEM_ABILITY_STAR:
        pObject = CreateItem(_eType, _vPos);
        break;

        // 타일/충돌체 타입들 (확장됨)
    case OBJECT_TYPE::TILE_GROUND:
    case OBJECT_TYPE::TILE_SPIKE:
    case OBJECT_TYPE::TILE_WATER:
    case OBJECT_TYPE::TILE_WARP_STAR:
    case OBJECT_TYPE::TILE_PLATFORM:      // 새로 추가
    case OBJECT_TYPE::TILE_LAVA:          // 새로 추가
    case OBJECT_TYPE::TILE_ONE_WAY:       // 새로 추가
    case OBJECT_TYPE::TILE_MOVING:        // 새로 추가
    case OBJECT_TYPE::TILE_BREAKABLE:     // 새로 추가
    case OBJECT_TYPE::TILE_INVISIBLE:     // 새로 추가
        pObject = CreateTile(_eType, _vPos);
        break;

        // 특수 객체들
    case OBJECT_TYPE::OBJECT_DOOR:
    case OBJECT_TYPE::OBJECT_SWITCH:
    case OBJECT_TYPE::OBJECT_MIRROR:
        pObject = CreateSpecialObject(_eType, _vPos);
        break;

    default:
        return nullptr;
    }

    if (pObject)
    {
        pObject->SetPos(_vPos);
        Vec2 vDefaultScale = GetDefaultScale(_eType);
        pObject->SetScale(vDefaultScale);
    }

    return pObject;
}

CObject* CObjectFactory::CreatePlayer(Vec2 _vPos)
{
    CPlayer* pPlayer = new CPlayer;
    pPlayer->SetPos(_vPos);
    pPlayer->SetScale(Vec2(16.f, 16.f));

    return pPlayer;
}

CObject* CObjectFactory::CreateMonster(OBJECT_TYPE _eMonsterType, Vec2 _vPos)
{
    CMonster* pMonster = new CMonster;
    pMonster->SetPos(_vPos);
    pMonster->SetMonsterType(_eMonsterType);  // 몬스터 타입 설정

    // 몬스터 타입에 따른 기본 설정
    switch (_eMonsterType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    case OBJECT_TYPE::MONSTER_GORDOS:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    case OBJECT_TYPE::MONSTER_SPARKY:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    default:
        pMonster->SetScale(Vec2(16.f, 16.f));
        break;
    }

    SetupMonsterAI(pMonster, _eMonsterType);
    return pMonster;
}

CObject* CObjectFactory::CreateItem(OBJECT_TYPE _eItemType, Vec2 _vPos)
{
    CItem* pItem = new CItem;
    pItem->SetItemType(_eItemType);

    switch (_eItemType)
    {
    case OBJECT_TYPE::ITEM_STAR:
        pItem->SetScale(Vec2(8.f, 8.f));  
        pItem->SetValue(100);
        break;
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:
        pItem->SetScale(Vec2(12.f, 16.f));
        pItem->SetValue(500);
        break;
    case OBJECT_TYPE::ITEM_1UP:
        pItem->SetScale(Vec2(16.f, 16.f));
        pItem->SetValue(1000);
        break;
    case OBJECT_TYPE::ITEM_ABILITY_STAR:
        pItem->SetScale(Vec2(12.f, 12.f));
        pItem->SetValue(200);
        break;
    default:
        pItem->SetScale(Vec2(8.f, 8.f));  
        pItem->SetValue(100);
        break;
    }

    SetupItemProperties(pItem, _eItemType);
    return pItem;
}

CObject* CObjectFactory::CreateTile(OBJECT_TYPE _eTileType, Vec2 _vPos)
{
    CTile* pTile = new CTile;
    pTile->SetTileType(_eTileType);  // 호환성용으로 유지

    // 모든 타일은 기본적으로 64x64 (한 타일 크기)
    pTile->SetScale(Vec2(64.f, 64.f));

    // OBJECT_TYPE을 새로운 COLLISION_TYPE으로 매핑
    COLLISION_TYPE collisionType = ConvertObjectTypeToCollisionType(_eTileType);
    pTile->SetCollisionType(collisionType);

    // 위치 설정
    pTile->SetPos(_vPos);

    return pTile;
}

CObject* CObjectFactory::CreateSpecialObject(OBJECT_TYPE _eObjectType, Vec2 _vPos)
{
    CSpecialObject* pObject = nullptr;

    switch (_eObjectType)
    {
    case OBJECT_TYPE::OBJECT_DOOR:
    {
        CDoor* pDoor = new CDoor;
        pDoor->SetPos(_vPos);

        // 기본 문 설정
        pDoor->SetTargetScene(SCENE_TYPE::STAGE_02);
        pDoor->SetTargetPosition(Vec2(100.f, 400.f));
        pDoor->SetDoorID(L"DefaultDoor");

        pObject = pDoor;
    }
        break;

    case OBJECT_TYPE::OBJECT_SWITCH:
    {
        // TODO: CSwitch 클래스 구현 후 생성
        CSpecialObject* pSwitch = new CSpecialObject;
        pSwitch->SetSpecialType(OBJECT_TYPE::OBJECT_SWITCH);
        pSwitch->SetScale(Vec2(48.f, 32.f));
        pSwitch->SetActive(false);
        pSwitch->SetInteractable(true);
        pObject = pSwitch;
    }
    break;

    case OBJECT_TYPE::OBJECT_MIRROR:
    {
        // TODO: CMirror 클래스 구현 후 생성  
        CSpecialObject* pMirror = new CSpecialObject;
        pMirror->SetSpecialType(OBJECT_TYPE::OBJECT_MIRROR);
        pMirror->SetScale(Vec2(96.f, 128.f));
        pMirror->SetActive(true);
        pMirror->SetInteractable(false);
        pObject = pMirror;
    }
    break;

    default:
        pObject->SetScale(Vec2(64.f, 64.f));
        pObject->SetActive(true);
        break;
    }

    return pObject;
}

const wchar_t* CObjectFactory::GetObjectTypeName(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
        // 플레이어
    case OBJECT_TYPE::PLAYER: return L"Player";

        // 몬스터들 (업데이트된 부분)
    case OBJECT_TYPE::MONSTER_WADDLE_DEE: return L"Waddle Dee";
    case OBJECT_TYPE::MONSTER_WADDLE_DOO: return L"Waddle Doo";
    case OBJECT_TYPE::MONSTER_BRONTO_BURT: return L"Bronto Burt";
    case OBJECT_TYPE::MONSTER_GORDOS: return L"Gordos";
    case OBJECT_TYPE::MONSTER_HOT_HEAD: return L"Hot Head";
    case OBJECT_TYPE::MONSTER_SPARKY: return L"Sparky";

        // 아이템들
    case OBJECT_TYPE::ITEM_STAR: return L"Star";
    case OBJECT_TYPE::ITEM_ENERGY_DRINK: return L"Energy Drink";
    case OBJECT_TYPE::ITEM_1UP: return L"1UP";
    case OBJECT_TYPE::ITEM_ABILITY_STAR: return L"Ability Star";

        // 충돌체/타일들 (새로운 이름들)
    case OBJECT_TYPE::TILE_GROUND: return L"Solid Ground";
    case OBJECT_TYPE::TILE_PLATFORM: return L"Platform";
    case OBJECT_TYPE::TILE_ONE_WAY: return L"One-Way Platform";
    case OBJECT_TYPE::TILE_SPIKE: return L"Spike";
    case OBJECT_TYPE::TILE_WATER: return L"Water";
    case OBJECT_TYPE::TILE_LAVA: return L"Lava";
    case OBJECT_TYPE::TILE_MOVING: return L"Moving Platform";
    case OBJECT_TYPE::TILE_BREAKABLE: return L"Breakable Block";
    case OBJECT_TYPE::TILE_INVISIBLE: return L"Invisible Wall";
    case OBJECT_TYPE::TILE_WARP_STAR: return L"Warp Star";

        // 특수 오브젝트들
    case OBJECT_TYPE::OBJECT_DOOR: return L"Door";
    case OBJECT_TYPE::OBJECT_SWITCH: return L"Switch";
    case OBJECT_TYPE::OBJECT_MIRROR: return L"Mirror";

    default: return L"Unknown";
    }
}


GROUP_TYPE CObjectFactory::GetObjectGroup(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
    case OBJECT_TYPE::PLAYER:
        return GROUP_TYPE::PLAYER;

    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
    case OBJECT_TYPE::MONSTER_GORDOS:
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
    case OBJECT_TYPE::MONSTER_SPARKY:
        return GROUP_TYPE::MONSTER;

    case OBJECT_TYPE::ITEM_STAR:
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:
    case OBJECT_TYPE::ITEM_1UP:
    case OBJECT_TYPE::ITEM_ABILITY_STAR:
        return GROUP_TYPE::ITEM;

    case OBJECT_TYPE::TILE_GROUND:
    case OBJECT_TYPE::TILE_PLATFORM:
    case OBJECT_TYPE::TILE_ONE_WAY:
    case OBJECT_TYPE::TILE_SPIKE:
    case OBJECT_TYPE::TILE_WATER:
    case OBJECT_TYPE::TILE_LAVA:
    case OBJECT_TYPE::TILE_MOVING:
    case OBJECT_TYPE::TILE_BREAKABLE:
    case OBJECT_TYPE::TILE_INVISIBLE:
    case OBJECT_TYPE::TILE_WARP_STAR:
        return GROUP_TYPE::TILE;

    case OBJECT_TYPE::OBJECT_DOOR:
    case OBJECT_TYPE::OBJECT_SWITCH:
    case OBJECT_TYPE::OBJECT_MIRROR:
        return GROUP_TYPE::SPECIAL;

    default:
        return GROUP_TYPE::DEFAULT;
    }
}

Vec2 CObjectFactory::GetDefaultScale(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
    case OBJECT_TYPE::PLAYER:               return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:   return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:   return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:  return Vec2(72.f, 64.f);
    case OBJECT_TYPE::MONSTER_GORDOS:       return Vec2(80.f, 80.f);
    case OBJECT_TYPE::MONSTER_HOT_HEAD:     return Vec2(64.f, 64.f);
    case OBJECT_TYPE::MONSTER_SPARKY:       return Vec2(64.f, 64.f);
    case OBJECT_TYPE::ITEM_STAR:            return Vec2(32.f, 32.f);
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:    return Vec2(48.f, 64.f);
    case OBJECT_TYPE::ITEM_1UP:             return Vec2(64.f, 64.f);
    case OBJECT_TYPE::ITEM_ABILITY_STAR:    return Vec2(48.f, 48.f);
    case OBJECT_TYPE::TILE_GROUND:          return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_PLATFORM:        return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_ONE_WAY:         return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_SPIKE:           return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_WATER:           return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_LAVA:            return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_MOVING:          return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_BREAKABLE:       return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_INVISIBLE:       return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_WARP_STAR:       return Vec2(64.f, 64.f);
    case OBJECT_TYPE::OBJECT_DOOR:          return Vec2(64.f, 128.f);
    case OBJECT_TYPE::OBJECT_SWITCH:        return Vec2(48.f, 32.f);
    case OBJECT_TYPE::OBJECT_MIRROR:        return Vec2(96.f, 128.f);
    default:                                return Vec2(64.f, 64.f);
    }
}

bool CObjectFactory::IsValidObjectType(OBJECT_TYPE _eType)
{
    return _eType >= OBJECT_TYPE::PLAYER && _eType < OBJECT_TYPE::END;
}

vector<OBJECT_TYPE> CObjectFactory::GetObjectTypesByCategory(const wstring& _strCategory)
{
    vector<OBJECT_TYPE> result;

    if (_strCategory == L"Monster")
    {
        result.push_back(OBJECT_TYPE::MONSTER_WADDLE_DEE);
        result.push_back(OBJECT_TYPE::MONSTER_WADDLE_DOO);
        result.push_back(OBJECT_TYPE::MONSTER_BRONTO_BURT);
        result.push_back(OBJECT_TYPE::MONSTER_GORDOS);
        result.push_back(OBJECT_TYPE::MONSTER_HOT_HEAD);
        result.push_back(OBJECT_TYPE::MONSTER_SPARKY);
    }
    else if (_strCategory == L"Item")
    {
        result.push_back(OBJECT_TYPE::ITEM_STAR);
        result.push_back(OBJECT_TYPE::ITEM_ENERGY_DRINK);
        result.push_back(OBJECT_TYPE::ITEM_1UP);
        result.push_back(OBJECT_TYPE::ITEM_ABILITY_STAR);
    }
    else if (_strCategory == L"Collision" || _strCategory == L"Tile")
    {
        result.push_back(OBJECT_TYPE::TILE_GROUND);
        result.push_back(OBJECT_TYPE::TILE_PLATFORM);
        result.push_back(OBJECT_TYPE::TILE_ONE_WAY);
        result.push_back(OBJECT_TYPE::TILE_SPIKE);
        result.push_back(OBJECT_TYPE::TILE_WATER);
        result.push_back(OBJECT_TYPE::TILE_LAVA);
        result.push_back(OBJECT_TYPE::TILE_MOVING);
        result.push_back(OBJECT_TYPE::TILE_BREAKABLE);
        result.push_back(OBJECT_TYPE::TILE_INVISIBLE);
        result.push_back(OBJECT_TYPE::TILE_WARP_STAR);
    }
    else if (_strCategory == L"Special")
    {
        result.push_back(OBJECT_TYPE::OBJECT_DOOR);
        result.push_back(OBJECT_TYPE::OBJECT_SWITCH);
        result.push_back(OBJECT_TYPE::OBJECT_MIRROR);
    }

    return result;
}

// GetAvailableCategories 함수 업데이트 (Collision 카테고리 추가)
vector<wstring> CObjectFactory::GetAvailableCategories()
{
    return { L"Monster", L"Item", L"Collision", L"Special" };
    // 기존 "Tile"은 "Collision"으로 변경 (더 명확한 의미)
}

// 새로 추가: OBJECT_TYPE을 COLLISION_TYPE으로 변환하는 함수
COLLISION_TYPE CObjectFactory::ConvertObjectTypeToCollisionType(OBJECT_TYPE _eObjectType)
{
    switch (_eObjectType)
    {
    case OBJECT_TYPE::TILE_GROUND:
        return COLLISION_TYPE::SOLID_GROUND;

    case OBJECT_TYPE::TILE_SPIKE:
        return COLLISION_TYPE::SPIKE;

    case OBJECT_TYPE::TILE_WATER:
        return COLLISION_TYPE::WATER;

    case OBJECT_TYPE::TILE_WARP_STAR:
        return COLLISION_TYPE::PLATFORM;  // 워프스타는 플랫폼으로 취급

        // 새로운 타일 타입들 추가 (기존 OBJECT_TYPE enum에 추가 필요)
    case OBJECT_TYPE::TILE_PLATFORM:
        return COLLISION_TYPE::PLATFORM;

    case OBJECT_TYPE::TILE_LAVA:
        return COLLISION_TYPE::LAVA;

    case OBJECT_TYPE::TILE_ONE_WAY:
        return COLLISION_TYPE::ONE_WAY_PLATFORM;

    case OBJECT_TYPE::TILE_MOVING:
        return COLLISION_TYPE::MOVING_PLATFORM;

    case OBJECT_TYPE::TILE_BREAKABLE:
        return COLLISION_TYPE::BREAKABLE_BLOCK;

    case OBJECT_TYPE::TILE_INVISIBLE:
        return COLLISION_TYPE::INVISIBLE_WALL;

    default:
        return COLLISION_TYPE::SOLID_GROUND;  // 기본값
    }
}

// 새로 추가: COLLISION_TYPE을 OBJECT_TYPE으로 변환하는 함수 (에디터용)
OBJECT_TYPE CObjectFactory::ConvertCollisionTypeToObjectType(COLLISION_TYPE _eCollisionType)
{
    switch (_eCollisionType)
    {
    case COLLISION_TYPE::SOLID_GROUND:
        return OBJECT_TYPE::TILE_GROUND;

    case COLLISION_TYPE::PLATFORM:
        return OBJECT_TYPE::TILE_PLATFORM;

    case COLLISION_TYPE::SPIKE:
        return OBJECT_TYPE::TILE_SPIKE;

    case COLLISION_TYPE::WATER:
        return OBJECT_TYPE::TILE_WATER;

    case COLLISION_TYPE::LAVA:
        return OBJECT_TYPE::TILE_LAVA;

    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        return OBJECT_TYPE::TILE_ONE_WAY;

    case COLLISION_TYPE::MOVING_PLATFORM:
        return OBJECT_TYPE::TILE_MOVING;

    case COLLISION_TYPE::BREAKABLE_BLOCK:
        return OBJECT_TYPE::TILE_BREAKABLE;

    case COLLISION_TYPE::INVISIBLE_WALL:
        return OBJECT_TYPE::TILE_INVISIBLE;

    default:
        return OBJECT_TYPE::TILE_GROUND;
    }
}

// CObjectFactory.cpp에 추가할 충돌체 관련 헬퍼 함수들

const wchar_t* CObjectFactory::GetCollisionTypeName(COLLISION_TYPE _eType)
{
    switch (_eType)
    {
    case COLLISION_TYPE::SOLID_GROUND:      return L"Solid Ground";
    case COLLISION_TYPE::PLATFORM:          return L"Platform";
    case COLLISION_TYPE::SPIKE:             return L"Spike";
    case COLLISION_TYPE::WATER:             return L"Water";
    case COLLISION_TYPE::LAVA:              return L"Lava";
    case COLLISION_TYPE::ONE_WAY_PLATFORM:  return L"One-Way Platform";
    case COLLISION_TYPE::MOVING_PLATFORM:   return L"Moving Platform";
    case COLLISION_TYPE::BREAKABLE_BLOCK:   return L"Breakable Block";
    case COLLISION_TYPE::INVISIBLE_WALL:    return L"Invisible Wall";
    default:                                return L"Unknown";
    }
}

COLORREF CObjectFactory::GetCollisionTypeColor(COLLISION_TYPE _eType)
{
    switch (_eType)
    {
    case COLLISION_TYPE::SOLID_GROUND:      return RGB(0, 0, 255);        // 파란색
    case COLLISION_TYPE::PLATFORM:          return RGB(0, 255, 0);        // 초록색
    case COLLISION_TYPE::SPIKE:             return RGB(255, 0, 0);        // 빨간색
    case COLLISION_TYPE::WATER:             return RGB(100, 200, 255);    // 연파란색
    case COLLISION_TYPE::LAVA:              return RGB(255, 100, 0);      // 주황색
    case COLLISION_TYPE::ONE_WAY_PLATFORM:  return RGB(100, 255, 100);    // 연초록색
    case COLLISION_TYPE::MOVING_PLATFORM:   return RGB(255, 0, 255);      // 보라색
    case COLLISION_TYPE::BREAKABLE_BLOCK:   return RGB(139, 69, 19);      // 황토색
    case COLLISION_TYPE::INVISIBLE_WALL:    return RGB(128, 128, 128);    // 회색
    default:                                return RGB(0, 0, 255);        // 기본 파란색
    }
}

vector<COLLISION_TYPE> CObjectFactory::GetAvailableCollisionTypes()
{
    vector<COLLISION_TYPE> result;

    result.push_back(COLLISION_TYPE::SOLID_GROUND);
    result.push_back(COLLISION_TYPE::PLATFORM);
    result.push_back(COLLISION_TYPE::ONE_WAY_PLATFORM);
    result.push_back(COLLISION_TYPE::SPIKE);
    result.push_back(COLLISION_TYPE::WATER);
    result.push_back(COLLISION_TYPE::LAVA);
    result.push_back(COLLISION_TYPE::MOVING_PLATFORM);
    result.push_back(COLLISION_TYPE::BREAKABLE_BLOCK);
    result.push_back(COLLISION_TYPE::INVISIBLE_WALL);

    return result;
}

void CObjectFactory::SetupMonsterAI(CObject* _pMonster, OBJECT_TYPE _eType)
{
    // TODO: 몬스터 타입별 AI 설정
    // 현재는 기본 CMonster 클래스만 있으므로 추후 확장

    switch (_eType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        // 기본 좌우 이동 AI (현재 CMonster 기본 동작)
        break;

    case OBJECT_TYPE::MONSTER_GORDOS:
        // 움직이지 않는 가시 - 추후 별도 클래스 필요
        break;

    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        // 날아다니는 AI - 추후 별도 클래스 필요
        break;

    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        // 불 공격 AI - 추후 별도 클래스 필요
        break;
    }
}

void CObjectFactory::SetupItemProperties(CObject* _pItem, OBJECT_TYPE _eType)
{
    // TODO: CItem 클래스 구현 후 아이템별 속성 설정
    // 점수, 효과, 애니메이션 등
}

void CObjectFactory::SetupTileProperties(CObject* _pTile, OBJECT_TYPE _eType)
{
    CTile* pTile = dynamic_cast<CTile*>(_pTile);
    if (!pTile) return;

    // 새로운 충돌체 시스템 사용
    COLLISION_TYPE collisionType = ConvertObjectTypeToCollisionType(_eType);
    pTile->SetCollisionType(collisionType);

    // 기본 오브젝트 타입도 설정 (호환성용)
    pTile->SetTileType(_eType);

    // *** 더 이상 시각적 텍스처 관련 처리는 하지 않음 ***
    // 시각적 타입 설정은 에디터에서 별도로 처리
    // pTile->SetVisualType(...);  // 더 이상 사용 안함

    // 충돌체 속성은 이미 SetCollisionType에서 자동으로 설정됨
}