#include "pch.h"
#include "CScene_Stage01.h"

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
#include "CStageMgr.h"

CScene_Stage01::CScene_Stage01()
    : m_strLevelFile(L"STAGE01")
    , m_pCurrentBackground(nullptr)
    , m_eCurrentBgType(BACKGROUND_TYPE::BACKGROUND1)
{
}

CScene_Stage01::~CScene_Stage01()
{
    // 배경은 CBackgroundMgr에서 관리하므로 여기서 삭제하지 않음
    m_pCurrentBackground = nullptr;
}

// === 핵심 생명주기 함수들 ===

void CScene_Stage01::Enter()
{
    // 기본 플레이어 생성 (레벨 로드에서 위치가 덮어써질 수 있음)
    CObject* pPlayer = new CPlayer;
    pPlayer->SetPos(Vec2(320.f, 320.f)); // 새로운 맵 크기에 맞는 기본 위치
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
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Kirby Clone - STAGE 01");
}

void CScene_Stage01::Exit()
{
    // 카메라 타겟 해제
    CCamera::GetInst()->SetTarget(nullptr);

    // 모든 객체 삭제
    DeleteAllObject();
}

void CScene_Stage01::Update()
{
    // 배경 업데이트
    if (m_pCurrentBackground)
    {
        m_pCurrentBackground->Update();
    }

    // 부모 클래스의 Update 호출 (모든 객체 업데이트)
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
}

void CScene_Stage01::Render(HDC _dc)
{
    // 배경 먼저 렌더링
    if (m_pCurrentBackground)
    {
        m_pCurrentBackground->Render(_dc);
    }

    CStageMgr::GetInst()->Render(_dc);

    // 부모 클래스의 Render 호출 (모든 객체 렌더링)
    CScene::Render(_dc);
}

// === 레벨 로드 관련 함수들 ===

void CScene_Stage01::LoadStageLevel(const wstring& _strFileName)
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strFullPath = strContentPath + L"level\\" + _strFileName + L".lvl";

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"rb");

    if (!pFile)
    {
        // 레벨 파일이 없으면 기본 레벨 생성
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level file not found! Using default level.");
        CreateDefaultLevel();
        return;
    }

    try
    {
        // 버전 확인
        int version;
        fread(&version, sizeof(int), 1, pFile);

        if (version != 1)
        {
            fclose(pFile);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Unsupported level version! Using default level.");
            CreateDefaultLevel();
            return;
        }

        // 레벨 이름 로드
        size_t nameLen;
        fread(&nameLen, sizeof(size_t), 1, pFile);
        if (nameLen > 0 && nameLen < 1000) // 안전성 검사
        {
            wstring levelName;
            levelName.resize(nameLen);
            fread(&levelName[0], sizeof(wchar_t), nameLen, pFile);
        }

        // 플레이어 스폰 위치
        Vec2 vPlayerSpawn;
        fread(&vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 배경 타입
        int backgroundType;
        fread(&backgroundType, sizeof(int), 1, pFile);

        // 경계 정보 (게임에서는 사용하지 않지만 읽어야 함)
        Vec2 vLevelBoundsMin, vLevelBoundsMax;
        float fGameOverY;
        fread(&vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fread(&vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fread(&fGameOverY, sizeof(float), 1, pFile);

        // 스테이지 이미지 정보
        int stageImageType;
        fread(&stageImageType, sizeof(int), 1, pFile);

        size_t stagePathLen;
        fread(&stagePathLen, sizeof(size_t), 1, pFile);
        if (stagePathLen > 0 && stagePathLen < 1000) // 안전성 검사
        {
            wstring stageImagePath;
            stageImagePath.resize(stagePathLen);
            fread(&stageImagePath[0], sizeof(wchar_t), stagePathLen, pFile);
        }

        // 스테이지 이미지 위치 (게임에서는 사용하지 않지만 읽어야 함)
        Vec2 vStageImagePos;
        fread(&vStageImagePos, sizeof(Vec2), 1, pFile);

        // 객체 개수
        size_t objCount;
        fread(&objCount, sizeof(size_t), 1, pFile);

        // 각 객체 생성
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

        // 로드된 정보 적용
        ApplyLoadedLevelData(vPlayerSpawn, (BACKGROUND_TYPE)backgroundType, (STAGE_IMAGE_TYPE)stageImageType);

        // 성공 메시지
        wchar_t szMsg[256];
        swprintf_s(szMsg, L"STAGE01 Loaded successfully (%d objects)", (int)objCount);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
    }
    catch (...)
    {
        fclose(pFile);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Error loading level! Using default level.");
        CreateDefaultLevel();
    }
}

CObject* CScene_Stage01::CreateObjectFromData(const tLevelObjectData& _objData)
{
    CObject* pObj = nullptr;

    switch (_objData.eGroupType)
    {
    case GROUP_TYPE::MONSTER:
    {
        OBJECT_TYPE monsterType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(monsterType, _objData.vPos);
    }
    break;

    case GROUP_TYPE::TILE:
    {
        OBJECT_TYPE tileType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(tileType, _objData.vPos);

        // 타일 시각 타입 및 충돌체 타입 복원
        if (pObj)
        {
            CTile* pTile = dynamic_cast<CTile*>(pObj);
            if (pTile)
            {
                // 시각적 타입 복원
                if (_objData.iTileVisualType >= 0)
                {
                    pTile->SetVisualType((TILE_VISUAL_TYPE)_objData.iTileVisualType);
                }

                // 충돌 타입 복원
                if (_objData.iCollisionType >= 0)
                {
                    pTile->SetCollisionType((COLLISION_TYPE)_objData.iCollisionType);
                }
            }
        }
    }
    break;

    case GROUP_TYPE::ITEM:
    {
        OBJECT_TYPE itemType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(itemType, _objData.vPos);
    }
    break;

    case GROUP_TYPE::SPECIAL:
    {
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

void CScene_Stage01::CreateDefaultLevel()
{
    // 기본 레벨 생성 (빈 레벨)
    // 플레이어는 이미 Enter()에서 생성되었으므로 위치만 조정

    // 플레이어 위치를 기본값으로 설정
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty() && vecPlayer[0])
    {
        vecPlayer[0]->SetPos(Vec2(320.f, 320.f)); // 새로운 맵 크기에 맞는 기본 위치
    }

    // 기본 배경 설정 (이미 InitializeBackgroundSystem에서 설정됨)
    // 기본 스테이지 이미지 설정
    CStageMgr::GetInst()->SetCurrentStageImage(STAGE_IMAGE_TYPE::STAGE_01);
}

// === 스테이지별 초기 설정 ===

void CScene_Stage01::InitializeStage()
{
    // 스테이지별 초기 설정
    // 예: 배경음악, 환경 설정, 특수 이벤트 등

    // 현재는 기본 설정만
}

// === 배경 시스템 관련 함수들 ===

void CScene_Stage01::InitializeBackgroundSystem()
{
    // 기본 배경 설정 (Stage01은 Background1 사용)
    m_eCurrentBgType = BACKGROUND_TYPE::BACKGROUND1;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(m_eCurrentBgType);
}

void CScene_Stage01::ChangeBackground(BACKGROUND_TYPE _eBgType)
{
    m_eCurrentBgType = _eBgType;
    m_pCurrentBackground = CBackgroundMgr::GetInst()->FindBackground(_eBgType);
}

// === 로드된 레벨 데이터 적용 ===

void CScene_Stage01::ApplyLoadedLevelData(Vec2 _vPlayerSpawn, BACKGROUND_TYPE _eBgType, STAGE_IMAGE_TYPE _eStageType)
{
    // 플레이어 스폰 위치 적용
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty() && vecPlayer[0])
    {
        vecPlayer[0]->SetPos(_vPlayerSpawn);
    }

    // 배경 적용
    ChangeBackground(_eBgType);

    // 스테이지 이미지 적용
    CStageMgr::GetInst()->SetCurrentStageImage(_eStageType);
}