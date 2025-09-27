#include "gamePCH.h"
#include "CScene_Stage02.h"

#include "CObject.h"
//#include "CPlayer.h"
#include "CMonster.h"
#include "CTile.h"
#include "CItem.h"
//#include "CObjectFactory.h"

#include "CKeyMgr.h"
#include "CCamera.h"
#include "CCore.h"
#include "CPathMgr.h"
#include "CEventMgr.h"
#include "CMonsterSpawnMgr.h"
#include "CSoundMgr.h"
#include "CBackgroundFactory.h"


CScene_Stage02::CScene_Stage02()
    : m_strLevelFile(L"STAGE02")
{
}

CScene_Stage02::~CScene_Stage02()
{
}

// === 핵심 생명주기 함수들 ===

void CScene_Stage02::Enter()
{
    //// 배경 오브젝트 생성 (Stage02는 Background2 사용)
    //CObject* pBackground = CBackgroundFactory::CreateBackgroundWithTexture(
    //    BACKGROUND_TYPE::BACKGROUND2,
    //    L"background\\background2.bmp",
    //    Vec2(320.f, 240.f)  // 화면 중앙
    //);
    //AddObject(pBackground, GROUP_TYPE::DEFAULT);

    //// 기본 플레이어 생성 (레벨 로드에서 위치가 덮어씌워질 수 있음)
    //CPlayer* pPlayer = new CPlayer;
    //pPlayer->SetPos(Vec2(320.f, 320.f)); // 샘플용 맵 크기에 맞는 기본 위치
    //pPlayer->SetScale(Vec2(100.f, 100.f));
    //AddObject(pPlayer, GROUP_TYPE::PLAYER);

    //// 저장된 플레이어 상태가 있으면 복원
    //pPlayer->LoadFromSavedData();

    //// 카메라가 플레이어를 따라가도록 설정
    //CCamera::GetInst()->SetTarget(pPlayer);
    //// 카메라를 즉시 플레이어 위치로 이동 (부드러운 이동 없이)
    //CCamera::GetInst()->SetLookAtImmediate(pPlayer->GetPos());

    //// 몬스터 스폰 매니저 초기화
    //CMonsterSpawnMgr::GetInst()->Initialize();
    //CMonsterSpawnMgr::GetInst()->SetPlayer(pPlayer);

    //// 스테이지 초기화
    //InitializeStage();

    //// 레벨 파일 로드 시도
    //LoadStageLevel(m_strLevelFile);

    //// 윈도우 타이틀 변경
    //SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Kirby Clone - STAGE 02");
    //
    //// 스테이지2 BGM 로드 및 재생
    //CSoundMgr::GetInst()->LoadSound(L"boss_battle", L"sound/boss_battle.mp3", SOUND_TYPE::BGM);
    //CSoundMgr::GetInst()->PlayBGM(L"boss_battle", true);
}

void CScene_Stage02::Exit()
{
    // BGM 정지
    CSoundMgr::GetInst()->StopBGM();
    
    // 몬스터 스폰 매니저 정리
    CMonsterSpawnMgr::GetInst()->Clear();
    

    // 모든 객체 삭제
    DeleteAllObject();
}

void CScene_Stage02::Update()
{
    // 몬스터 스폰 매니저 업데이트
    CMonsterSpawnMgr::GetInst()->Update();

    // 부모 클래스의 Update 호출 (모든 객체 업데이트)
    CScene::Update();

    // 스테이지별 특수 로직 (필요시 추가)

    // Enter키로 STAGE01로 이동 (테스트용)
    if (KEY_TAP(KEY::ENTER))
    {
        tEvent event(EVENT_TYPE::SCENE_CHANGE, 0, (DWORD_PTR)SCENE_TYPE::STAGE_01);
        CEventMgr::GetInst()->AddEvent(event);
    }
}

void CScene_Stage02::Render(HDC _dc)
{
    // 부모 클래스의 Render 호출 (모든 객체 렌더링)
    CScene::Render(_dc);
}

// === 레벨 로드 관련 함수들 ===

void CScene_Stage02::LoadStageLevel(const wstring& _strFileName)
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

        // 레벨 이름 로드 (사용하지 않지만 파일 포맷 맞추기 위해)
        size_t nameLen;
        fread(&nameLen, sizeof(size_t), 1, pFile);
        if (nameLen > 0 && nameLen < 1000)
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

        // 경계 정보 (현재는 사용하지 않음)
        Vec2 vLevelBoundsMin, vLevelBoundsMax;
        float fGameOverY;
        fread(&vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fread(&vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fread(&fGameOverY, sizeof(float), 1, pFile);

        // 스테이지 이미지 타입
        int stageImageType;
        fread(&stageImageType, sizeof(int), 1, pFile);

        // 스테이지 이미지 경로
        size_t stagePathLen;
        fread(&stagePathLen, sizeof(size_t), 1, pFile);
        if (stagePathLen > 0 && stagePathLen < 1000)
        {
            wstring stageImagePath;
            stageImagePath.resize(stagePathLen);
            fread(&stageImagePath[0], sizeof(wchar_t), stagePathLen, pFile);
        }

        // 스테이지 이미지 위치
        Vec2 vStageImagePos;
        fread(&vStageImagePos, sizeof(Vec2), 1, pFile);

        // 객체 개수
        size_t objCount;
        fread(&objCount, sizeof(size_t), 1, pFile);

        // 객체 데이터 로드
        size_t createdObjectCount = 0;
        for (size_t i = 0; i < objCount; ++i)
        {
            tLevelObjectData objData;
            fread(&objData, sizeof(tLevelObjectData), 1, pFile);

            CObject* pObj = CreateObjectFromData(objData);
            if (pObj)
            {
                AddObject(pObj, objData.eGroupType);
                createdObjectCount++;
            }
            // 몬스터는 스폰 매니저에 등록되므로 실제 생성된 객체 수는 다름
        }

        fclose(pFile);

        // 로드된 데이터 적용
        ApplyLoadedLevelData(vPlayerSpawn, (BACKGROUND_TYPE)backgroundType);
        


        // 성공 메시지
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"STAGE02 loaded successfully (%d objects, %d monster spawns)", 
                   (int)createdObjectCount, (int)CMonsterSpawnMgr::GetInst()->GetSpawnDataCount());
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
    catch (...)
    {
        fclose(pFile);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Error occurred while loading! Using default level.");
        CreateDefaultLevel();
    }
}

CObject* CScene_Stage02::CreateObjectFromData(const tLevelObjectData& _objData)
{
    return nullptr;
    //CObject* pObj = nullptr;

    //switch (_objData.eGroupType)
    //{
    //case GROUP_TYPE::MONSTER:
    //{
    //    OBJECT_TYPE monsterType = (OBJECT_TYPE)_objData.iSubType;
    //    
    //    // 보스 몬스터는 직접 생성 (항상 활성화 상태 유지)
    //    if (monsterType == OBJECT_TYPE::MONSTER_WHISPY_WOODS)
    //    {
    //        pObj = CObjectFactory::CreateObject(monsterType, _objData.vPos);
    //        if (pObj)
    //        {
    //            CMonster* pMonster = dynamic_cast<CMonster*>(pObj);
    //            if (pMonster)
    //            {
    //                // 게임 모드로 설정
    //                pMonster->SetDirection((int)_objData.fDirection);
    //                pMonster->ChangeState(MONSTER_STATE::IDLE);
    //            }
    //        }
    //    }
    //    else
    //    {
    //        // 일반 몬스터는 스폰 매니저로 관리
    //        CMonsterSpawnMgr::GetInst()->AddSpawnData(_objData.vPos, monsterType, _objData.fDirection);
    //        pObj = nullptr; // 씬에 직접 추가되지 않도록
    //    }
    //}
    //break;

    //case GROUP_TYPE::TILE:
    //{
    //    OBJECT_TYPE tileType = (OBJECT_TYPE)_objData.iSubType;
    //    pObj = CObjectFactory::CreateObject(tileType, _objData.vPos);

    //    // 타일 시각 타입 및 충돌체 타입 복원
    //    if (pObj)
    //    {
    //        CTile* pTile = dynamic_cast<CTile*>(pObj);
    //        if (pTile)
    //        {
    //            // 타일 타입을 먼저 설정 (중요!)
    //            pTile->SetTileType((OBJECT_TYPE)_objData.iSubType);
    //            pTile->SetType((OBJECT_TYPE)_objData.iSubType);  // CObject의 기본 타입도 설정
    //            
    //            // 시각적 타입 복원
    //            if (_objData.iTileVisualType >= 0)
    //            {
    //                pTile->SetVisualType((TILE_VISUAL_TYPE)_objData.iTileVisualType);
    //            }

    //            // 충돌 타입 복원
    //            if (_objData.iCollisionType >= 0)
    //            {
    //                pTile->SetCollisionType((COLLISION_TYPE)_objData.iCollisionType);
    //            }
    //            
    //            // 트리거 타일인 경우 카메라 좌표 복원
    //            if (pTile->GetTileType() == OBJECT_TYPE::TILE_TRIGGER)
    //            {
    //                pTile->SetBossLockPosition(_objData.vBossLockPos);
    //            }
    //        }
    //    }
    //}
    //break;

    //case GROUP_TYPE::ITEM:
    //{
    //    OBJECT_TYPE itemType = (OBJECT_TYPE)_objData.iSubType;
    //    pObj = CObjectFactory::CreateObject(itemType, _objData.vPos);
    //}
    //break;

    //case GROUP_TYPE::SPECIAL:
    //{
    //    OBJECT_TYPE specialType = (OBJECT_TYPE)_objData.iSubType;
    //    pObj = CObjectFactory::CreateObject(specialType, _objData.vPos);
    //}
    //break;

    //default:
    //    return nullptr;
    //}

    //if (pObj)
    //{
    //    pObj->SetPos(_objData.vPos);
    //    pObj->SetScale(_objData.vScale);
    //}

    //return pObj;
}

void CScene_Stage02::CreateDefaultLevel()
{
    // 기본 레벨 생성 (빈 레벨)
    // 플레이어는 이미 Enter()에서 생성되었으므로 위치만 조정

    // 플레이어 위치를 기본값으로 설정
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty() && vecPlayer[0])
    {
        vecPlayer[0]->SetPos(Vec2(320.f, 320.f)); // 샘플용 맵 크기에 맞는 기본 위치
    }

    // 기본 배경 설정 (이미 InitializeBackgroundSystem에서 설정됨)
    
    // 기본 스테이지 경계 설정 (레벨 데이터가 없을 때만 사용)
    Vec2 vDefaultMapSize = Vec2(4096.f, 640.f);

}

void CScene_Stage02::ApplyLoadedLevelData(Vec2 _vPlayerSpawn, BACKGROUND_TYPE _eBgType)
{
    // 플레이어 스폰 위치 적용
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty() && vecPlayer[0])
    {
        vecPlayer[0]->SetPos(_vPlayerSpawn);
    }

}

// === 스테이지별 초기 설정 ===

void CScene_Stage02::InitializeStage()
{
    // 스테이지별 초기 설정
    // 예: 배경음악, 환경 설정, 특수 이벤트 등

    // 현재는 기본 설정만
}
