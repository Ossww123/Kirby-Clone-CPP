#include "pch.h"
#include "CScene_Stage02.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"
#include "CTile.h"
#include "CItem.h"
#include "CSpecialObject.h"
#include "CObjectFactory.h"

#include "CKeyMgr.h"
#include "CCamera.h"
#include "CCore.h"
#include "CPathMgr.h"
#include "CEventMgr.h"
#include "CTileMgr.h"
#include "CBackground.h"
#include "CBackgroundMgr.h"

CScene_Stage02::CScene_Stage02()
    : m_strLevelFile(L"STAGE02")  // 기본 레벨 파일명
{
}

CScene_Stage02::~CScene_Stage02()
{
}

void CScene_Stage02::Enter()
{
    // 기본 플레이어 생성 (레벨 로드에서 위치가 덮어씌워질 수 있음)
    CObject* pPlayer = new CPlayer;
    pPlayer->SetPos(Vec2(640.f, 400.f));
    pPlayer->SetScale(Vec2(100.f, 100.f));
    AddObject(pPlayer, GROUP_TYPE::PLAYER);

    // 카메라가 플레이어를 따라가도록 설정
    CCamera::GetInst()->SetTarget(pPlayer);

    // 배경 시스템 초기화
    InitializeBackgroundSystem();

    // 스테이지 초기화
    InitializeStage();

    // 레벨 파일 로드 시도
    LoadStageLevel(m_strLevelFile);

    // 윈도우 타이틀 변경
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Kirby Clone - STAGE 02");
}

void CScene_Stage02::Exit()
{
    // 카메라 타겟 해제
    CCamera::GetInst()->SetTarget(nullptr);

    // 모든 오브젝트 삭제
    DeleteAllObject();
}

void CScene_Stage02::Update()
{
    // 배경 업데이트 추가
    if (m_pCurrentBackground)
    {
        m_pCurrentBackground->Update();
    }

    // 부모 클래스의 Update 호출 (모든 오브젝트 업데이트)
    CScene::Update();

    // 스테이지별 특수 로직 (필요시 추가)

    // ESC키로 Tool Scene으로 복귀 (디버깅용)
    if (KEY_TAP(KEY::ESC))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::TOOL);
        CEventMgr::GetInst()->AddEvent(event);
    }

    // Enter키로 START 씬으로 복귀
    if (KEY_TAP(KEY::ENTER))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::START);
        CEventMgr::GetInst()->AddEvent(event);
    }

    // Backspace키로 STAGE_01로 복귀
    if (KEY_TAP(KEY::BACK))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::STAGE_01);
        CEventMgr::GetInst()->AddEvent(event);
    }
}

void CScene_Stage02::Render(HDC _dc)
{
    // 1. 배경 먼저 렌더링
    if (m_pCurrentBackground)
    {
        m_pCurrentBackground->Render(_dc);
    }

    // 2. 부모 클래스의 Render 호출 (모든 오브젝트 렌더링)
    CScene::Render(_dc);
}

void CScene_Stage02::InitializeBackgroundSystem()
{
    // Stage02는 Background2 사용
    m_eCurrentBgType = BACKGROUND_TYPE::BACKGROUND2;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(m_eCurrentBgType);
}

void CScene_Stage02::ChangeBackground(BACKGROUND_TYPE _eBgType)
{
    m_eCurrentBgType = _eBgType;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(_eBgType);
}

void CScene_Stage02::InitializeStage()
{
    // STAGE02만의 특별한 초기 설정
    // 예: 특별한 물리 법칙, 특수 이벤트 등

    // 현재는 기본 설정만
}

void CScene_Stage02::LoadStageLevel(const wstring& _strFileName)
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strFullPath = strContentPath + L"level\\" + _strFileName + L".lvl";

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"rb");

    if (!pFile)
    {
        // 레벨 파일이 없으면 기본 레벨 생성
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"STAGE02 Level file not found! Using default level.");
        CreateDefaultLevel();
        return;
    }

    // 파일에서 데이터 읽기
    tLevelData levelData;

    // 버전 확인
    fread(&levelData.iVersion, sizeof(int), 1, pFile);

    if (levelData.iVersion < 1 || levelData.iVersion > 3)
    {
        fclose(pFile);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Unsupported STAGE02 level version! Using default level.");
        CreateDefaultLevel();
        return;
    }

    // 레벨 이름
    size_t nameLen;
    fread(&nameLen, sizeof(size_t), 1, pFile);
    wchar_t* szName = new wchar_t[nameLen + 1];
    fread(szName, sizeof(wchar_t), nameLen, pFile);
    szName[nameLen] = L'\0';
    levelData.strLevelName = szName;
    delete[] szName;

    // 플레이어 스폰 위치
    fread(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

    // 배경 타입 정보 (버전 2 이상에서만)
    if (levelData.iVersion >= 2)
    {
        fread(&levelData.iBackgroundType, sizeof(int), 1, pFile);
    }

    // === 추가: 버전 3의 경계 정보 읽기 ===
    if (levelData.iVersion >= 3)
    {
        fread(&levelData.vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fread(&levelData.vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fread(&levelData.fGameOverY, sizeof(float), 1, pFile);
    }

    // 오브젝트 개수
    size_t objCount;
    fread(&objCount, sizeof(size_t), 1, pFile);

    // 각 오브젝트 생성
    for (size_t i = 0; i < objCount; ++i)
    {
        tLevelObjectData objData;
        fread(&objData, sizeof(tLevelObjectData), 1, pFile);

        CObject* pObj = CreateObjectFromData(objData);
        if (pObj)
        {
            AddObject(pObj, objData.eGroupType);
        }
    }

    fclose(pFile);

    // 성공 메시지
    wchar_t szMsg[256];
    swprintf_s(szMsg, L"STAGE02 Loaded: %s (%d objects)", levelData.strLevelName.c_str(), (int)objCount);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
}

CObject* CScene_Stage02::CreateObjectFromData(const tLevelObjectData& _objData)
{
    CObject* pObj = nullptr;

    switch (_objData.eGroupType)
    {
    case GROUP_TYPE::MONSTER:
    {
        // 몬스터 타입에 따라 생성
        OBJECT_TYPE monsterType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(monsterType, _objData.vPos);
    }
    break;

    case GROUP_TYPE::TILE:
    {
        // 타일 타입에 따라 생성
        OBJECT_TYPE tileType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(tileType, _objData.vPos);

        // 타일 시각 타입 적용
        if (pObj && _objData.iTileVisualType >= 0)
        {
            CTile* pTile = dynamic_cast<CTile*>(pObj);
            if (pTile)
            {
                TILE_VISUAL_TYPE eVisualType = (TILE_VISUAL_TYPE)_objData.iTileVisualType;
                pTile->SetVisualType(eVisualType);

                // 타일 매니저를 통해 적절한 텍스처와 속성 설정
                CTileMgr::GetInst()->SetupTileProperties(pTile, eVisualType);
            }
        }
    }
    break;

    case GROUP_TYPE::ITEM:
    {
        // 아이템 타입에 따라 생성
        OBJECT_TYPE itemType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(itemType, _objData.vPos);
    }
    break;

    case GROUP_TYPE::SPECIAL:
    {
        // 특수 오브젝트 타입에 따라 생성
        OBJECT_TYPE specialType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(specialType, _objData.vPos);
    }
    break;

    default:
        return nullptr;
    }

    if (pObj)
    {
        pObj->SetPos(_objData.vPos);
        pObj->SetScale(_objData.vScale);
    }

    return pObj;
}

void CScene_Stage02::CreateDefaultLevel()
{
    // STAGE02 기본 레벨 생성 (레벨 에디터로 제대로 만들기 전까지 임시용)

    // 첫 번째 플랫폼
    for (int x = 0; x < 8; ++x)
    {
        CObject* pTile = CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND,
            Vec2(100.f + x * 64.f, 500.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 두 번째 플랫폼 (위쪽)
    for (int x = 0; x < 6; ++x)
    {
        CObject* pTile = CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND,
            Vec2(300.f + x * 64.f, 350.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 몬스터들 배치
    CObject* pMonster1 = CObjectFactory::CreateObject(OBJECT_TYPE::MONSTER_WADDLE_DEE,
        Vec2(400.f, 400.f));
    AddObject(pMonster1, GROUP_TYPE::MONSTER);

    CObject* pMonster2 = CObjectFactory::CreateObject(OBJECT_TYPE::MONSTER_GORDOS,
        Vec2(600.f, 250.f));
    AddObject(pMonster2, GROUP_TYPE::MONSTER);

    // 아이템 배치
    CObject* pItem = CObjectFactory::CreateObject(OBJECT_TYPE::ITEM_STAR,
        Vec2(500.f, 300.f));
    AddObject(pItem, GROUP_TYPE::ITEM);

    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"STAGE02 Default level created!");
}