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

    // 오브젝트 타입에 따라 적절한 생성 함수 호출
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

        // 타일 타입들
    case OBJECT_TYPE::TILE_GROUND:
    case OBJECT_TYPE::TILE_SPIKE:
    case OBJECT_TYPE::TILE_WATER:
    case OBJECT_TYPE::TILE_WARP_STAR:
        pObject = CreateTile(_eType, _vPos);
        break;

        // 특수 오브젝트들
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
    pPlayer->SetScale(Vec2(64.f, 64.f)); // 4배 확대된 크기

    return pPlayer;
}

CObject* CObjectFactory::CreateMonster(OBJECT_TYPE _eMonsterType, Vec2 _vPos)
{
    CMonster* pMonster = new CMonster;
    pMonster->SetPos(_vPos);

    // 몬스터 타입에 따른 기본 설정
    switch (_eMonsterType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        pMonster->SetScale(Vec2(64.f, 64.f));
        // TODO: 와들디 전용 AI 설정
        break;

    case OBJECT_TYPE::MONSTER_GORDOS:
        pMonster->SetScale(Vec2(80.f, 80.f)); // 조금 더 큰 크기
        // TODO: 고르도스 전용 AI 설정 (움직이지 않는 가시)
        break;

    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        pMonster->SetScale(Vec2(72.f, 64.f)); // 가로로 조금 긴 형태
        // TODO: 브론토 버트 전용 AI 설정 (날아다님)
        break;

    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        pMonster->SetScale(Vec2(64.f, 64.f));
        // TODO: 핫 헤드 전용 AI 설정 (불 공격)
        break;

    default:
        pMonster->SetScale(Vec2(64.f, 64.f));
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
        pItem->SetScale(Vec2(32.f, 32.f));
        pItem->SetValue(100);
        break;

    case OBJECT_TYPE::ITEM_ENERGY_DRINK:
        pItem->SetScale(Vec2(48.f, 64.f));
        pItem->SetValue(500);
        break;

    case OBJECT_TYPE::ITEM_1UP:
        pItem->SetScale(Vec2(64.f, 64.f));
        pItem->SetValue(1000);
        break;

    case OBJECT_TYPE::ITEM_ABILITY_STAR:
        pItem->SetScale(Vec2(48.f, 48.f));
        pItem->SetValue(200);
        break;

    default:
        pItem->SetScale(Vec2(32.f, 32.f));
        pItem->SetValue(100);
        break;
    }

    SetupItemProperties(pItem, _eItemType);
    return pItem;
}

CObject* CObjectFactory::CreateTile(OBJECT_TYPE _eTileType, Vec2 _vPos)
{
    CTile* pTile = new CTile;
    pTile->SetTileType(_eTileType);

    // 모든 타일은 기본적으로 64x64 (한 타일 크기)
    pTile->SetScale(Vec2(64.f, 64.f));

    // 1. 콜라이더 추가 (충돌 처리용)
    if (pTile->IsSolid() && !pTile->IsDecorative())
    {
        pTile->CreateCollider();
        pTile->GetCollider()->SetScale(Vec2(64.f, 64.f));
    }

    // 2. 비주얼 타입 설정 및 텍스처 로드
    TILE_VISUAL_TYPE visualType = TILE_VISUAL_TYPE::GRASS_PLATFORM;

    switch (_eTileType)
    {
    case OBJECT_TYPE::TILE_GROUND:
        visualType = TILE_VISUAL_TYPE::GRASS_PLATFORM;
        pTile->SetSolid(true);
        pTile->SetHarmful(false);
        break;

    case OBJECT_TYPE::TILE_SPIKE:
        visualType = TILE_VISUAL_TYPE::SPIKE;
        pTile->SetSolid(true);
        pTile->SetHarmful(true);
        break;

    case OBJECT_TYPE::TILE_WATER:
        visualType = TILE_VISUAL_TYPE::WATER;
        pTile->SetSolid(false);
        pTile->SetHarmful(false);
        break;

    case OBJECT_TYPE::TILE_WARP_STAR:
        pTile->SetSolid(false);
        pTile->SetHarmful(false);
        break;
    }

    // 3. 타일 설정 적용
    pTile->SetVisualType(visualType);
    CTileMgr::GetInst()->SetupTileProperties(pTile, visualType);

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
    case OBJECT_TYPE::PLAYER:               return L"Player";
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:   return L"Waddle Dee";
    case OBJECT_TYPE::MONSTER_GORDOS:       return L"Gordos";
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:  return L"Bronto Burt";
    case OBJECT_TYPE::MONSTER_HOT_HEAD:     return L"Hot Head";
    case OBJECT_TYPE::ITEM_STAR:            return L"Star";
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:    return L"Energy Drink";
    case OBJECT_TYPE::ITEM_1UP:             return L"1UP";
    case OBJECT_TYPE::ITEM_ABILITY_STAR:    return L"Ability Star";
    case OBJECT_TYPE::TILE_GROUND:          return L"Ground Tile";
    case OBJECT_TYPE::TILE_SPIKE:           return L"Spike Tile";
    case OBJECT_TYPE::TILE_WATER:           return L"Water Tile";
    case OBJECT_TYPE::TILE_WARP_STAR:       return L"Warp Star";
    case OBJECT_TYPE::OBJECT_DOOR:          return L"Door";
    case OBJECT_TYPE::OBJECT_SWITCH:        return L"Switch";
    case OBJECT_TYPE::OBJECT_MIRROR:        return L"Mirror";
    default:                                return L"Unknown";
    }
}


GROUP_TYPE CObjectFactory::GetObjectGroup(OBJECT_TYPE _eType)
{
    switch (_eType)
    {
    case OBJECT_TYPE::PLAYER:
        return GROUP_TYPE::PLAYER;

    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
    case OBJECT_TYPE::MONSTER_GORDOS:
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        return GROUP_TYPE::MONSTER;

    case OBJECT_TYPE::ITEM_STAR:
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:
    case OBJECT_TYPE::ITEM_1UP:
    case OBJECT_TYPE::ITEM_ABILITY_STAR:
        return GROUP_TYPE::ITEM;

    case OBJECT_TYPE::TILE_GROUND:
    case OBJECT_TYPE::TILE_SPIKE:
    case OBJECT_TYPE::TILE_WATER:
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
    case OBJECT_TYPE::MONSTER_GORDOS:       return Vec2(80.f, 80.f);
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:  return Vec2(72.f, 64.f);
    case OBJECT_TYPE::MONSTER_HOT_HEAD:     return Vec2(64.f, 64.f);
    case OBJECT_TYPE::ITEM_STAR:            return Vec2(32.f, 32.f);
    case OBJECT_TYPE::ITEM_ENERGY_DRINK:    return Vec2(48.f, 64.f);
    case OBJECT_TYPE::ITEM_1UP:             return Vec2(64.f, 64.f);
    case OBJECT_TYPE::ITEM_ABILITY_STAR:    return Vec2(48.f, 48.f);
    case OBJECT_TYPE::TILE_GROUND:          return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_SPIKE:           return Vec2(64.f, 64.f);
    case OBJECT_TYPE::TILE_WATER:           return Vec2(64.f, 64.f);
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
        result.push_back(OBJECT_TYPE::MONSTER_GORDOS);
        result.push_back(OBJECT_TYPE::MONSTER_BRONTO_BURT);
        result.push_back(OBJECT_TYPE::MONSTER_HOT_HEAD);
    }
    else if (_strCategory == L"Item")
    {
        result.push_back(OBJECT_TYPE::ITEM_STAR);
        result.push_back(OBJECT_TYPE::ITEM_ENERGY_DRINK);
        result.push_back(OBJECT_TYPE::ITEM_1UP);
        result.push_back(OBJECT_TYPE::ITEM_ABILITY_STAR);
    }
    else if (_strCategory == L"Tile")
    {
        result.push_back(OBJECT_TYPE::TILE_GROUND);
        result.push_back(OBJECT_TYPE::TILE_SPIKE);
        result.push_back(OBJECT_TYPE::TILE_WATER);
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

vector<wstring> CObjectFactory::GetAvailableCategories()
{
    return { L"Monster", L"Item", L"Tile", L"Special" };
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
    // TODO: CTile 클래스 구현 후 타일별 속성 설정
    // 충돌 여부, 텍스처, 특수 효과 등
}