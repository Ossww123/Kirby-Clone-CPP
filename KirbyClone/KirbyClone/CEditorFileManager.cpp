#include "pch.h"
#include "CEditorFileManager.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"
#include "CEditorCameraController.h"
#include "CEditorToolbar.h"

#include "CScene.h"
#include "CObjectFactory.h"
#include "CPathMgr.h"
#include "CCore.h"
#include "CTile.h"
#include "CTileMgr.h"
#include "CStageMgr.h"
#include "CBackgroundMgr.h"
#include "CStageImage.h"
#include "CTexture.h"

#include <commdlg.h>

CEditorFileManager::CEditorFileManager()
    : m_pEditorCore(nullptr)
    , m_pScene(nullptr)
    , m_strQuickSaveFile(L"quicksave")
{
}

CEditorFileManager::~CEditorFileManager()
{
}

void CEditorFileManager::Initialize(CEditorCore* _pCore, CScene* _pScene)
{
    m_pEditorCore = _pCore;
    m_pScene = _pScene;

    // 레벨 디렉토리 확인 및 생성
    EnsureLevelDirectoryExists();
}

// === 파일 저장/로드 인터페이스 ===

void CEditorFileManager::SaveAsDialog()
{
    wstring strFileName;
    if (ShowSaveDialog(strFileName))
    {
        SaveLevel(strFileName);
    }
}

void CEditorFileManager::OpenDialog()
{
    wstring strFileName;
    if (ShowOpenDialog(strFileName))
    {
        LoadLevel(strFileName);
    }
}

void CEditorFileManager::QuickSave()
{
    SaveLevel(m_strQuickSaveFile);
}

void CEditorFileManager::QuickLoad()
{
    LoadLevel(m_strQuickSaveFile);
}

// === 레벨 파일 처리 ===

void CEditorFileManager::SaveLevel(const wstring& _strFileName)
{
    // 레벨 데이터 생성
    tLevelData levelData = CreateLevelData(_strFileName);

    // 파일 경로 생성
    wstring strFullPath = GetFullLevelPath(_strFileName);

    // 레벨 디렉토리 확인
    if (!EnsureLevelDirectoryExists())
    {
        ShowErrorMessage(L"Failed to create level directory!");
        return;
    }

    // 파일에 저장
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"wb");

    if (!pFile)
    {
        ShowErrorMessage(L"Failed to create save file!");
        return;
    }

    try
    {
        // 버전 정보 (v1)
        int version = 1;
        fwrite(&version, sizeof(int), 1, pFile);

        // 레벨 이름 길이 및 이름
        size_t nameLen = levelData.strLevelName.length();
        fwrite(&nameLen, sizeof(size_t), 1, pFile);
        fwrite(levelData.strLevelName.c_str(), sizeof(wchar_t), nameLen, pFile);

        // 플레이어 스폰 위치
        fwrite(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 배경 타입 정보
        fwrite(&levelData.iBackgroundType, sizeof(int), 1, pFile);

        // 경계 정보 저장
        fwrite(&levelData.vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fwrite(&levelData.vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fwrite(&levelData.fGameOverY, sizeof(float), 1, pFile);

        // 스테이지 이미지 정보
        int stageImageType = (int)levelData.eStageType;
        fwrite(&stageImageType, sizeof(int), 1, pFile);

        size_t stagePathLen = levelData.strStageImagePath.length();
        fwrite(&stagePathLen, sizeof(size_t), 1, pFile);
        if (stagePathLen > 0)
        {
            fwrite(levelData.strStageImagePath.c_str(), sizeof(wchar_t), stagePathLen, pFile);
        }

        // 스테이지 이미지 위치
        fwrite(&levelData.vStageImagePos, sizeof(Vec2), 1, pFile);

        // 객체 개수
        size_t objCount = levelData.vecObjects.size();
        fwrite(&objCount, sizeof(size_t), 1, pFile);

        // 각 객체 데이터
        for (const auto& objData : levelData.vecObjects)
        {
            fwrite(&objData, sizeof(tLevelObjectData), 1, pFile);
        }

        fclose(pFile);

        // 성공 메시지
        ShowSuccessMessage(_strFileName + L" saved successfully!", (int)objCount);
    }
    catch (...)
    {
        fclose(pFile);
        ShowErrorMessage(L"Error occurred while saving!");
    }
}

void CEditorFileManager::LoadLevel(const wstring& _strFileName)
{
    wstring strFullPath = GetFullLevelPath(_strFileName);

    // 파일 존재 확인
    if (!ValidateLevelFile(strFullPath))
    {
        if (_strFileName == m_strQuickSaveFile)
        {
            // 퀵로드 파일이 없으면 기본 레벨 생성
            CreateDefaultLevel();
            ShowSuccessMessage(L"Default level created (no quicksave found)");
        }
        else
        {
            ShowErrorMessage(L"Level file not found!");
        }
        return;
    }

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"rb");

    if (!pFile)
    {
        ShowErrorMessage(L"Failed to open level file!");
        return;
    }

    try
    {
        // 버전 확인
        int version;
        fread(&version, sizeof(int), 1, pFile);

        if (!IsValidVersion(version))
        {
            fclose(pFile);
            ShowErrorMessage(L"Unsupported level file version!");
            return;
        }

        // 씬 클리어
        ClearScene();

        tLevelData levelData;
        levelData.iVersion = version;

        // 레벨 이름 로드
        size_t nameLen;
        fread(&nameLen, sizeof(size_t), 1, pFile);
        if (nameLen > 0 && nameLen < 1000) // 안전성 검사
        {
            levelData.strLevelName.resize(nameLen);
            fread(&levelData.strLevelName[0], sizeof(wchar_t), nameLen, pFile);
        }

        // 플레이어 스폰 위치
        fread(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 배경 타입
        fread(&levelData.iBackgroundType, sizeof(int), 1, pFile);

        // 경계 정보
        fread(&levelData.vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fread(&levelData.vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fread(&levelData.fGameOverY, sizeof(float), 1, pFile);

        // 스테이지 이미지 타입
        int stageImageType;
        fread(&stageImageType, sizeof(int), 1, pFile);
        levelData.eStageType = (STAGE_IMAGE_TYPE)stageImageType;

        // 스테이지 이미지 경로
        size_t stagePathLen;
        fread(&stagePathLen, sizeof(size_t), 1, pFile);
        if (stagePathLen > 0 && stagePathLen < 1000) // 안전성 검사
        {
            levelData.strStageImagePath.resize(stagePathLen);
            fread(&levelData.strStageImagePath[0], sizeof(wchar_t), stagePathLen, pFile);
        }

        // 스테이지 이미지 위치
        fread(&levelData.vStageImagePos, sizeof(Vec2), 1, pFile);

        // 객체 개수
        size_t objCount;
        fread(&objCount, sizeof(size_t), 1, pFile);

        // 객체 데이터 로드
        levelData.vecObjects.reserve(objCount);
        for (size_t i = 0; i < objCount; ++i)
        {
            tLevelObjectData objData;
            fread(&objData, sizeof(tLevelObjectData), 1, pFile);
            levelData.vecObjects.push_back(objData);
        }

        fclose(pFile);

        // 로드된 데이터 적용
        ApplyLoadedLevelData(levelData);

        // 성공 메시지
        ShowSuccessMessage(_strFileName + L" loaded successfully (v" + to_wstring(version) + L")", (int)objCount);
    }
    catch (...)
    {
        fclose(pFile);
        ShowErrorMessage(L"Error occurred while loading!");
    }
}

// === 레벨 데이터 생성/처리 ===

tLevelData CEditorFileManager::CreateLevelData(const wstring& _strLevelName)
{
    tLevelData levelData;
    levelData.strLevelName = _strLevelName;
    levelData.iVersion = 1; // 버전 1

    // 플레이어 스폰 위치
    if (m_pEditorCore->GetObjectManager())
    {
        levelData.vPlayerSpawn = m_pEditorCore->GetObjectManager()->GetPlayerSpawnPos();
    }

    // 배경 정보
    if (m_pEditorCore->GetObjectManager())
    {
        levelData.iBackgroundType = (int)m_pEditorCore->GetObjectManager()->GetCurrentBackgroundType();
    }

    // 경계 정보 (새로운 기본값)
    levelData.vLevelBoundsMin = Vec2(0.f, 0.f);
    levelData.vLevelBoundsMax = Vec2(4096.f, 640.f);  // 새로운 기본 맵 크기
    levelData.fGameOverY = 680.f;                      // 새로운 게임오버 Y

    // 스테이지 이미지 정보 (버전 1에서는 기본 스테이지만)
    levelData.eStageType = CStageMgr::GetInst()->GetCurrentStageType();

    // 커스텀 이미지인 경우 경로 저장
    if (levelData.eStageType == STAGE_IMAGE_TYPE::CUSTOM)
    {
        CStageImage* pCurrentStage = CStageMgr::GetInst()->GetCurrentStageImage();
        if (pCurrentStage && pCurrentStage->GetStageTexture())
        {
            levelData.strStageImagePath = pCurrentStage->GetStageTexture()->GetRelativePath();
        }
    }
    else
    {
        levelData.strStageImagePath = L"";
    }

    // 스테이지 이미지 위치 (기본값)
    levelData.vStageImagePos = Vec2(0.f, 0.f);

    // 모든 그룹의 객체들 수집
    CollectSceneObjects(levelData);

    return levelData;
}

void CEditorFileManager::CollectSceneObjects(tLevelData& _levelData)
{
    if (!m_pScene)
        return;

    // 각 그룹별로 객체들 수집
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        GROUP_TYPE eGroupType = (GROUP_TYPE)i;

        // 플레이어 그룹은 제외 (스폰 위치만 저장)
        if (eGroupType == GROUP_TYPE::PLAYER)
            continue;

        const vector<CObject*>& vecObj = m_pScene->GetGroupObject(eGroupType);

        for (CObject* pObj : vecObj)
        {
            if (pObj)
            {
                tLevelObjectData objData = CreateObjectData(pObj, eGroupType);
                _levelData.vecObjects.push_back(objData);
            }
        }
    }
}

void CEditorFileManager::ApplyLoadedLevelData(const tLevelData& _levelData)
{
    // 플레이어 스폰 위치 설정
    if (m_pEditorCore->GetObjectManager())
    {
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(_levelData.vPlayerSpawn);
    }

    // 배경 설정
    if (m_pEditorCore->GetObjectManager())
    {
        BACKGROUND_TYPE bgType = (BACKGROUND_TYPE)_levelData.iBackgroundType;
        m_pEditorCore->GetObjectManager()->ChangeBackground(bgType);
    }

    // 스테이지 이미지 시스템 적용
    if (_levelData.eStageType == STAGE_IMAGE_TYPE::CUSTOM && !_levelData.strStageImagePath.empty())
    {
        // 커스텀 이미지 로드 (향후 구현)
        // CStageMgr::GetInst()->LoadCustomStageImage(_levelData.strStageImagePath);
    }
    else
    {
        // 기본 스테이지 이미지 설정
        CStageMgr::GetInst()->SetCurrentStageImage(_levelData.eStageType);
    }

    // 객체들 생성
    for (const auto& objData : _levelData.vecObjects)
    {
        CObject* pObj = CreateObjectFromData(objData);
        if (pObj)
        {
            GROUP_TYPE eGroup = objData.eGroupType;
            m_pScene->AddObject(pObj, eGroup);
        }
    }

    // 경계 정보 적용
    ApplyLevelBounds(_levelData);
}

// === 객체 변환 ===

CObject* CEditorFileManager::CreateObjectFromData(const tLevelObjectData& _objData)
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

        // 타일 시각 타입 및 충돌체 타입 적용
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

tLevelObjectData CEditorFileManager::CreateObjectData(CObject* _pObj, GROUP_TYPE _eGroupType)
{
    tLevelObjectData objData;
    objData.eGroupType = _eGroupType;
    objData.vPos = _pObj->GetPos();
    objData.vScale = _pObj->GetScale();
    objData.iSubType = 0; // 기본값

    // 타일의 경우 시각 타입과 충돌 타입 정보도 저장
    if (objData.eGroupType == GROUP_TYPE::TILE)
    {
        CTile* pTile = dynamic_cast<CTile*>(_pObj);
        if (pTile)
        {
            objData.iSubType = (int)pTile->GetTileType();
            objData.iTileVisualType = (int)pTile->GetVisualType();
            objData.iCollisionType = (int)pTile->GetCollisionType();
        }
    }
    // 몬스터의 경우 몬스터 타입 정보 저장
    else if (objData.eGroupType == GROUP_TYPE::MONSTER)
    {
        objData.iSubType = (int)_pObj->GetType();
    }
    // 아이템의 경우 아이템 타입 정보 저장
    else if (objData.eGroupType == GROUP_TYPE::ITEM)
    {
        objData.iSubType = (int)_pObj->GetType();
    }
    // 특수 객체의 경우
    else if (objData.eGroupType == GROUP_TYPE::SPECIAL)
    {
        objData.iSubType = (int)_pObj->GetType();
    }

    return objData;
}

// === 씬 관리 ===

void CEditorFileManager::ClearScene()
{
    // 플레이어를 제외한 모든 객체 삭제
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        const vector<CObject*>& vecObj = m_pScene->GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j])
            {
                vecObj[j]->SetDead();
            }
        }
    }
    // 선택 해제
    m_pEditorCore->DeselectObject();
}

void CEditorFileManager::CreateDefaultLevel()
{
    // 기본 레벨 생성 (빈 레벨)
    ClearScene();

    // 플레이어 스폰 위치 초기화 (새로운 맵 크기에 맞게)
    m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(Vec2(320.f, 320.f));

    // 기본 배경 설정
    m_pEditorCore->GetObjectManager()->SetCurrentBackgroundType(BACKGROUND_TYPE::BACKGROUND1);

    // 기본 카메라 경계 설정 (새로운 맵 크기)
    CEditorCameraController* pCameraController = m_pEditorCore->GetCameraController();
    if (pCameraController)
    {
        pCameraController->SetCameraBounds(Vec2(0.f, 0.f), Vec2(4096.f, 640.f));
    }
}

void CEditorFileManager::ApplyLevelBounds(const tLevelData& _levelData)
{
    CEditorCameraController* pCameraController = m_pEditorCore->GetCameraController();
    if (pCameraController)
    {
        // 로드된 레벨의 경계 정보 적용
        pCameraController->SetCameraBounds(_levelData.vLevelBoundsMin, _levelData.vLevelBoundsMax);

        // 카메라를 레벨 경계 내 적절한 위치로 이동
        Vec2 vSafePos = Vec2(
            max(_levelData.vLevelBoundsMin.x + 480.f, 480.f), // 화면 절반 고려
            min(_levelData.vLevelBoundsMax.y - 320.f, 320.f)  // 화면 절반 고려
        );
        pCameraController->SetCameraPosition(vSafePos);
    }

    // 툴바에 맵 크기 정보 업데이트
    if (m_pEditorCore->GetToolbar())
    {
        Vec2 vMapSize = _levelData.vLevelBoundsMax - _levelData.vLevelBoundsMin;
        m_pEditorCore->GetToolbar()->SetMapSize(vMapSize);
    }
}

// === 파일 경로 유틸리티 ===

wstring CEditorFileManager::GetLevelDirectory() const
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    return strContentPath + L"level\\";
}

wstring CEditorFileManager::GetFullLevelPath(const wstring& _strFileName) const
{
    return GetLevelDirectory() + _strFileName + L".lvl";
}

bool CEditorFileManager::EnsureLevelDirectoryExists()
{
    wstring strLevelDir = GetLevelDirectory();
    return CreateDirectory(strLevelDir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// === 파일 대화상자 헬퍼 ===

bool CEditorFileManager::ShowSaveDialog(wstring& _strFileName)
{
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };
    wchar_t szFileTitle[260] = { 0 };

    // 기본 파일명 설정
    wcscpy_s(szFile, L"NewLevel.lvl");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = CCore::GetInst()->GetMainHwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = GetLevelDirectory().c_str();
    ofn.lpstrFilter = L"Kirby Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"레벨 파일 저장";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"lvl";

    if (GetSaveFileName(&ofn))
    {
        _strFileName = szFileTitle;

        // 확장자 제거
        size_t dotPos = _strFileName.rfind(L'.');
        if (dotPos != wstring::npos)
        {
            _strFileName = _strFileName.substr(0, dotPos);
        }

        return true;
    }

    return false;
}

bool CEditorFileManager::ShowOpenDialog(wstring& _strFileName)
{
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };
    wchar_t szFileTitle[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = CCore::GetInst()->GetMainHwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = GetLevelDirectory().c_str();
    ofn.lpstrFilter = L"Kirby Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"레벨 파일 열기";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn))
    {
        _strFileName = szFileTitle;

        // 확장자 제거
        size_t dotPos = _strFileName.rfind(L'.');
        if (dotPos != wstring::npos)
        {
            _strFileName = _strFileName.substr(0, dotPos);
        }

        return true;
    }

    return false;
}

// === 파일 검증 ===

bool CEditorFileManager::ValidateLevelFile(const wstring& _strFilePath)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, _strFilePath.c_str(), L"rb");

    if (!pFile)
        return false;

    // 최소한의 파일 크기 검사
    fseek(pFile, 0, SEEK_END);
    long fileSize = ftell(pFile);
    fclose(pFile);

    return fileSize > sizeof(int); // 최소한 버전 정보는 있어야 함
}

bool CEditorFileManager::IsValidVersion(int _iVersion) const
{
    return _iVersion == 1; // 현재는 버전 1만 지원
}

// === 메시지 시스템 ===

void CEditorFileManager::ShowErrorMessage(const wstring& _strMessage)
{
    SetWindowText(CCore::GetInst()->GetMainHwnd(), (_strMessage + L" - Error").c_str());
}

void CEditorFileManager::ShowSuccessMessage(const wstring& _strMessage, int _iObjectCount)
{
    wstring strFullMessage = _strMessage;
    if (_iObjectCount >= 0)
    {
        strFullMessage += L" (" + to_wstring(_iObjectCount) + L" objects)";
    }
    SetWindowText(CCore::GetInst()->GetMainHwnd(), strFullMessage.c_str());
}