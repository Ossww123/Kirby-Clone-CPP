#include "gamePCH.h"
#include "CEditorObjectManager.h"
#include "CEditorCore.h"

#include "CScene.h"
#include "CScene_Tool.h"
#include "CObjectFactory.h"
#include "CBackground.h"
#include "CBackgroundMgr.h"
#include "CTile.h"
#include "CTileMgr.h"
#include "CCore.h"
#include "CMonster.h"
#include "CRigidBody.h"

CEditorObjectManager::CEditorObjectManager()
    : m_pEditorCore(nullptr)
    , m_eCurrentObjectType(OBJECT_TYPE::MONSTER_WADDLE_DEE)
    , m_iCurrentSubType(0)
    , m_vecCurrentCategory{}
    , m_pCurrentBackground(nullptr)
    , m_eCurrentBgType(BACKGROUND_TYPE::BACKGROUND1)
    , m_vecBackgroundTypes{}
    , m_eCurrentTileVisual(TILE_VISUAL_TYPE::TRANSPARENT_BLOCK)
    , m_vecTileVisualTypes{}
    , m_iTileVisualIndex(0)
    , m_vPlayerSpawnPos(Vec2(640.f, 400.f))
    , m_bShowPlayerSpawn(true)
    , m_bPlayerSpawnMode(false)
{
}

CEditorObjectManager::~CEditorObjectManager()
{
    // 배경은 CBackgroundMgr에서 관리되므로 여기서 삭제하지 않음
    m_pCurrentBackground = nullptr;
}

void CEditorObjectManager::Initialize(CEditorCore* _pCore)
{
    m_pEditorCore = _pCore;

    // 기본 설정
    m_eCurrentObjectType = OBJECT_TYPE::MONSTER_WADDLE_DEE;
    ChangeObjectCategory(L"Monster"); // 기본 카테고리 설정

    // 배경 시스템 초기화
    InitializeBackgroundSystem();

    // 타일 비주얼 시스템 초기화
    InitializeTileVisualSystem();

    // 플레이어 스폰 초기화
    m_vPlayerSpawnPos = Vec2(640.f, 400.f);
    m_bShowPlayerSpawn = true;
    m_bPlayerSpawnMode = false;
}

void CEditorObjectManager::Update()
{
    // 배경 업데이트
    if (m_pCurrentBackground)
    {
        m_pCurrentBackground->Update();
    }
}

void CEditorObjectManager::PlaceObject(Vec2 _vPos)
{
    // 팩토리를 통해서 오브젝트 생성
    CObject* pObject = CObjectFactory::CreateObject(m_eCurrentObjectType, _vPos);

    if (!pObject)
    {
        return;
    }

    // 몬스터인 경우 에디터 모드 설정
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_MONSTER)
    {
        CMonster* pMonster = dynamic_cast<CMonster*>(pObject);
        if (pMonster)
        {
            pMonster->SetEditorMode(true);
            pMonster->ChangeState(MONSTER_STATE::EDITOR_IDLE);
            
            // 중력 비활성화
            if (pMonster->GetRigidBody())
            {
                pMonster->GetRigidBody()->SetUseGravity(false);
            }
        }
    }

    // 타일인 경우 비주얼 타일 설정
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
    {
        CTile* pTile = dynamic_cast<CTile*>(pObject);
        if (pTile)
        {
            pTile->SetVisualType(m_eCurrentTileVisual);

            // 타일 매니저에서 타일 비주얼 속성 설정
            CTileMgr::GetInst()->SetupTileProperties(pTile, m_eCurrentTileVisual);
        }
    }

    // 적절한 그룹에 추가
    GROUP_TYPE eGroup = CObjectFactory::GetObjectGroup(m_eCurrentObjectType);
    m_pEditorCore->GetWorkingScene()->AddObject(pObject, eGroup);
}

void CEditorObjectManager::DeleteObjectAtPos(Vec2 _vPos)
{
    // 클릭한 위치에서 오브젝트 찾기
    CObject* pTargetObj = FindObjectAtPos(_vPos);

    if (!pTargetObj)
    {
        return;
    }

    // 선택된 오브젝트와 같은 객체라면 선택 해제
    if (m_pEditorCore->GetSelectedObject() == pTargetObj)
    {
        m_pEditorCore->DeselectObject();
    }

    // 씬에서 해당 오브젝트 삭제
    CScene* pScene = m_pEditorCore->GetWorkingScene();
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(pScene->GetGroupObject((GROUP_TYPE)i));

        auto iter = find(vecObj.begin(), vecObj.end(), pTargetObj);
        if (iter != vecObj.end())
        {
            delete pTargetObj;
            vecObj.erase(iter);
            return;
        }
    }
}

CObject* CEditorObjectManager::FindObjectAtPos(Vec2 _vPos)
{
    // 모든 그룹에서 오브젝트 검사 (플레이어 제외)
    CScene* pScene = m_pEditorCore->GetWorkingScene();

    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = pScene->GetGroupObject((GROUP_TYPE)i);

        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            Vec2 vObjPos = vecObj[j]->GetPos();
            Vec2 vObjScale = vecObj[j]->GetScale();

            // AABB 검사 (사각형 충돌 검사)
            if (_vPos.x >= vObjPos.x - vObjScale.x / 2.f &&
                _vPos.x <= vObjPos.x + vObjScale.x / 2.f &&
                _vPos.y >= vObjPos.y - vObjScale.y / 2.f &&
                _vPos.y <= vObjPos.y + vObjScale.y / 2.f)
            {
                return vecObj[j];
            }
        }
    }

    return nullptr;
}

void CEditorObjectManager::ChangeObjectCategory(const wstring& _strCategory)
{
    m_vecCurrentCategory = CObjectFactory::GetObjectTypesByCategory(_strCategory);

    if (!m_vecCurrentCategory.empty())
    {
        m_iCurrentSubType = 0;
        m_eCurrentObjectType = m_vecCurrentCategory[0];
        
        // 타일 카테고리로 변경될 때 비주얼 타입도 초기화
        if (_strCategory == L"Tile" || _strCategory == L"Collision")
        {
            if (m_eCurrentObjectType == OBJECT_TYPE::TILE_TRIGGER)
            {
                m_eCurrentTileVisual = TILE_VISUAL_TYPE::BOSS_TRIGGER;
            }
            else if (m_eCurrentObjectType == OBJECT_TYPE::TILE_GROUND)
            {
                m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
            }
        }
    }
}

void CEditorObjectManager::NextObjectInCategory()
{
    if (m_vecCurrentCategory.empty()) return;

    m_iCurrentSubType = (m_iCurrentSubType + 1) % m_vecCurrentCategory.size();
    m_eCurrentObjectType = m_vecCurrentCategory[m_iCurrentSubType];
    
    // 타일 모드에서 비주얼 타입 업데이트
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
    {
        if (m_eCurrentObjectType == OBJECT_TYPE::TILE_TRIGGER)
        {
            m_eCurrentTileVisual = TILE_VISUAL_TYPE::BOSS_TRIGGER;
        }
        else if (m_eCurrentObjectType == OBJECT_TYPE::TILE_GROUND)
        {
            m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
        }
    }
}

void CEditorObjectManager::PrevObjectInCategory()
{
    if (m_vecCurrentCategory.empty()) return;

    m_iCurrentSubType = (m_iCurrentSubType - 1 + m_vecCurrentCategory.size()) % m_vecCurrentCategory.size();
    m_eCurrentObjectType = m_vecCurrentCategory[m_iCurrentSubType];
    
    // 타일 모드에서 비주얼 타입 업데이트
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
    {
        if (m_eCurrentObjectType == OBJECT_TYPE::TILE_TRIGGER)
        {
            m_eCurrentTileVisual = TILE_VISUAL_TYPE::BOSS_TRIGGER;
        }
        else if (m_eCurrentObjectType == OBJECT_TYPE::TILE_GROUND)
        {
            m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
        }
    }
}

const wchar_t* CEditorObjectManager::GetCurrentObjectName() const
{
    return CObjectFactory::GetObjectTypeName(m_eCurrentObjectType);
}

void CEditorObjectManager::SetCurrentSubType(int index)
{
    if (index >= 0 && index < (int)m_vecCurrentCategory.size())
    {
        m_iCurrentSubType = index;
        m_eCurrentObjectType = m_vecCurrentCategory[index];
        
        // 타일 모드에서 오브젝트 타입에 따라 비주얼 타입도 업데이트
        if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
        {
            if (m_eCurrentObjectType == OBJECT_TYPE::TILE_TRIGGER)
            {
                m_eCurrentTileVisual = TILE_VISUAL_TYPE::BOSS_TRIGGER;
            }
            else if (m_eCurrentObjectType == OBJECT_TYPE::TILE_GROUND)
            {
                m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
            }
        }
    }
}

void CEditorObjectManager::InitializeBackgroundSystem()
{
    // 사용 가능한 모든 배경 타입들 초기화
    m_vecBackgroundTypes.push_back(BACKGROUND_TYPE::BACKGROUND1);
    m_vecBackgroundTypes.push_back(BACKGROUND_TYPE::BACKGROUND2);
    m_vecBackgroundTypes.push_back(BACKGROUND_TYPE::BACKGROUND3);

    // 기본 배경 설정
    m_eCurrentBgType = BACKGROUND_TYPE::BACKGROUND1;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(m_eCurrentBgType);
}

void CEditorObjectManager::ChangeBackground(BACKGROUND_TYPE _eBgType)
{
    m_eCurrentBgType = _eBgType;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(_eBgType);
}

void CEditorObjectManager::NextBackground()
{
    if (m_vecBackgroundTypes.empty()) return;

    // 현재 배경의 인덱스 찾기
    int currentIndex = 0;
    for (size_t i = 0; i < m_vecBackgroundTypes.size(); ++i)
    {
        if (m_vecBackgroundTypes[i] == m_eCurrentBgType)
        {
            currentIndex = (int)i;
            break;
        }
    }

    // 다음 배경으로 변경
    int nextIndex = (currentIndex + 1) % m_vecBackgroundTypes.size();
    ChangeBackground(m_vecBackgroundTypes[nextIndex]);
}

void CEditorObjectManager::PrevBackground()
{
    if (m_vecBackgroundTypes.empty()) return;

    // 현재 배경의 인덱스 찾기
    int currentIndex = 0;
    for (size_t i = 0; i < m_vecBackgroundTypes.size(); ++i)
    {
        if (m_vecBackgroundTypes[i] == m_eCurrentBgType)
        {
            currentIndex = (int)i;
            break;
        }
    }

    // 이전 배경으로 변경
    int prevIndex = (currentIndex - 1 + m_vecBackgroundTypes.size()) % m_vecBackgroundTypes.size();
    ChangeBackground(m_vecBackgroundTypes[prevIndex]);
}

const wchar_t* CEditorObjectManager::GetBackgroundName(BACKGROUND_TYPE _eType) const
{
    return CBackgroundMgr::GetInst()->GetBackgroundName(_eType);
}

void CEditorObjectManager::InitializeTileVisualSystem()
{
    // 사용 가능한 모든 타일 비주얼 타입들 초기화
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::TRANSPARENT_BLOCK);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::BOSS_TRIGGER);
    /*m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::GRASS_PLATFORM);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::DIRT_BLOCK);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::STONE_BLOCK);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::GRASS_BLOCK);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::TREE);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::FLOWER);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::FENCE);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::SPIKE);
    m_vecTileVisualTypes.push_back(TILE_VISUAL_TYPE::WATER);*/

    // 기본 타일 비주얼 타입 설정
    m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
    m_iTileVisualIndex = 0;
}

void CEditorObjectManager::NextTileVisual()
{
    if (m_vecTileVisualTypes.empty()) return;

    m_iTileVisualIndex = (m_iTileVisualIndex + 1) % m_vecTileVisualTypes.size();
    m_eCurrentTileVisual = m_vecTileVisualTypes[m_iTileVisualIndex];
}

void CEditorObjectManager::PrevTileVisual()
{
    if (m_vecTileVisualTypes.empty()) return;

    m_iTileVisualIndex = (m_iTileVisualIndex - 1 + m_vecTileVisualTypes.size()) % m_vecTileVisualTypes.size();
    m_eCurrentTileVisual = m_vecTileVisualTypes[m_iTileVisualIndex];
}

const wchar_t* CEditorObjectManager::GetTileVisualName(TILE_VISUAL_TYPE _eType) const
{
    switch (_eType)
    {
    case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK: return L"Transparent Block";
    case TILE_VISUAL_TYPE::BOSS_TRIGGER:      return L"Boss Trigger";
    /*case TILE_VISUAL_TYPE::GRASS_PLATFORM:   return L"Grass Platform";
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
    case TILE_VISUAL_TYPE::BRIDGE:           return L"Bridge";*/
    default:                                 return L"Unknown";
    }
}

void CEditorObjectManager::ClearAllObjects ( )
{
    // Scene의 DeleteAllObject 함수 호출
    CScene_Tool* pToolScene = dynamic_cast< CScene_Tool* >( m_pEditorCore->GetWorkingScene ( ) );
    if ( pToolScene )
    {
        pToolScene->ClearAllObjects ( );  // public 함수 호출

        // 선택된 오브젝트도 해제
        m_pEditorCore->DeselectObject ( );
    }
}

void CEditorObjectManager::ResetToDefault ( )
{
    // 오브젝트 모두 삭제
    ClearAllObjects ( );

    // 기본 설정으로 복원
    m_eCurrentObjectType = OBJECT_TYPE::MONSTER_WADDLE_DEE;
    ChangeObjectCategory ( L"Monster" );

    // 플레이어 스폰 위치 복원
    m_vPlayerSpawnPos = Vec2 ( 640.f , 400.f );
    m_bShowPlayerSpawn = true;

    // 기본 배경으로 복원
    ChangeBackground ( BACKGROUND_TYPE::BACKGROUND1 );

    // 기본 타일 속성으로 복원
    m_eCurrentTileVisual = TILE_VISUAL_TYPE::TRANSPARENT_BLOCK;
    m_iTileVisualIndex = 0;
}

int CEditorObjectManager::GetTotalObjectCount() const
{
    int totalCount = 0;

    if (m_pEditorCore && m_pEditorCore->GetWorkingScene())
    {
        CScene* pScene = m_pEditorCore->GetWorkingScene();

        for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
        {
            const vector<CObject*>& vecObj = pScene->GetGroupObject((GROUP_TYPE)i);
            totalCount += (int)vecObj.size();
        }
    }

    return totalCount;
}