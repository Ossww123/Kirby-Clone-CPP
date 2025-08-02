#include "pch.h"
#include "CEditorFileManager.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"

#include "CScene.h"
#include "CObjectFactory.h"
#include "CPathMgr.h"
#include "CCore.h"
#include "CTile.h"
#include "CTileMgr.h"

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
        // 버전 정보
        fwrite(&levelData.iVersion, sizeof(int), 1, pFile);

        // 레벨 이름 길이 및 이름
        size_t nameLen = levelData.strLevelName.length();
        fwrite(&nameLen, sizeof(size_t), 1, pFile);
        fwrite(levelData.strLevelName.c_str(), sizeof(wchar_t), nameLen, pFile);

        // 플레이어 스폰 위치
        fwrite(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 배경 타입 정보
        fwrite(&levelData.iBackgroundType, sizeof(int), 1, pFile);

        // 오브젝트 개수
        size_t objCount = levelData.vecObjects.size();
        fwrite(&objCount, sizeof(size_t), 1, pFile);

        // 각 오브젝트 데이터
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
        // 기존 오브젝트들 삭제
        ClearScene();

        // 파일에서 데이터 읽기
        tLevelData levelData;

        // 버전 확인
        fread(&levelData.iVersion, sizeof(int), 1, pFile);
        if (!IsValidVersion(levelData.iVersion))
        {
            fclose(pFile);
            ShowErrorMessage(L"Unsupported level version!");
            return;
        }

        // 레벨 이름
        size_t nameLen;
        fread(&nameLen, sizeof(size_t), 1, pFile);
        if (nameLen > 256) // 보안 검사
        {
            fclose(pFile);
            ShowErrorMessage(L"Invalid level file format!");
            return;
        }

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

            // 배경 변경 적용
            BACKGROUND_TYPE eBgType = (BACKGROUND_TYPE)levelData.iBackgroundType;
            if (eBgType >= BACKGROUND_TYPE::GREEN_HILL && eBgType < BACKGROUND_TYPE::END)
            {
                m_pEditorCore->GetObjectManager()->ChangeBackground(eBgType);
            }
        }
        else
        {
            // 구버전 파일의 경우 기본 배경 사용
            m_pEditorCore->GetObjectManager()->ChangeBackground(BACKGROUND_TYPE::GREEN_HILL);
        }

        // 플레이어 스폰 위치 설정
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPosition(levelData.vPlayerSpawn);

        // 오브젝트 개수
        size_t objCount;
        fread(&objCount, sizeof(size_t), 1, pFile);
        if (objCount > 10000) // 보안 검사
        {
            fclose(pFile);
            ShowErrorMessage(L"Invalid object count in level file!");
            return;
        }

        // 각 오브젝트 생성
        int successCount = 0;
        for (size_t i = 0; i < objCount; ++i)
        {
            tLevelObjectData objData;
            fread(&objData, sizeof(tLevelObjectData), 1, pFile);

            CObject* pObj = CreateObjectFromData(objData);
            if (pObj)
            {
                m_pScene->AddObject(pObj, objData.eGroupType);
                successCount++;
            }
        }

        fclose(pFile);

        // 성공 메시지
        ShowSuccessMessage(_strFileName + L" loaded successfully!", successCount);
    }
    catch (...)
    {
        fclose(pFile);
        ShowErrorMessage(L"Error occurred while loading!");
    }
}

tLevelData CEditorFileManager::CreateLevelData(const wstring& _strLevelName)
{
    tLevelData levelData;
    levelData.strLevelName = _strLevelName;
    levelData.iVersion = 2; // 현재 버전
    levelData.iBackgroundType = (int)m_pEditorCore->GetObjectManager()->GetCurrentBackgroundType();
    levelData.vPlayerSpawn = m_pEditorCore->GetObjectManager()->GetPlayerSpawnPos();

    // 모든 오브젝트 데이터 수집 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = m_pScene->GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j] && !vecObj[j]->IsDead()) // 유효하고 살아있는 오브젝트만
            {
                tLevelObjectData objData = CreateObjectData(vecObj[j], (GROUP_TYPE)i);
                levelData.vecObjects.push_back(objData);
            }
        }
    }

    return levelData;
}

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

        // 타일 시각 타입 적용
        if (pObj)
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

    // 타일의 경우 시각 타입 정보도 저장
    if (objData.eGroupType == GROUP_TYPE::TILE)
    {
        CTile* pTile = dynamic_cast<CTile*>(_pObj);
        if (pTile)
        {
            objData.iSubType = (int)pTile->GetTileType();
            objData.iTileVisualType = (int)pTile->GetVisualType();
        }
    }
    // 몬스터의 경우 몬스터 타입 정보 저장
    else if (objData.eGroupType == GROUP_TYPE::MONSTER)
    {
        // 현재는 모든 몬스터가 WADDLE_DEE이므로 기본값 사용
        objData.iSubType = (int)OBJECT_TYPE::MONSTER_WADDLE_DEE;
    }

    return objData;
}

void CEditorFileManager::ClearScene()
{
    // 기존 오브젝트들 삭제 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(m_pScene->GetGroupObject((GROUP_TYPE)i));
        for (CObject* pObj : vecObj)
        {
            if (pObj)
            {
                delete pObj;
            }
        }
        vecObj.clear();
    }

    // 선택 해제
    m_pEditorCore->DeselectObject();
}

void CEditorFileManager::CreateDefaultLevel()
{
    // 기본 지면 타일들 생성 (플랫폼)
    for (int x = 0; x < 10; ++x)
    {
        CObject* pTile = CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND,
            Vec2(100.f + x * 64.f, 500.f));

        if (pTile)
        {
            CTile* pTileComponent = dynamic_cast<CTile*>(pTile);
            if (pTileComponent)
            {
                pTileComponent->SetVisualType(TILE_VISUAL_TYPE::GRASS_PLATFORM);
                CTileMgr::GetInst()->SetupTileProperties(pTileComponent, TILE_VISUAL_TYPE::GRASS_PLATFORM);
            }
            m_pScene->AddObject(pTile, GROUP_TYPE::TILE);
        }
    }

    // 기본 몬스터 몇 마리 배치
    CObject* pMonster1 = CObjectFactory::CreateObject(OBJECT_TYPE::MONSTER_WADDLE_DEE,
        Vec2(300.f, 400.f));
    if (pMonster1) m_pScene->AddObject(pMonster1, GROUP_TYPE::MONSTER);

    CObject* pMonster2 = CObjectFactory::CreateObject(OBJECT_TYPE::MONSTER_WADDLE_DEE,
        Vec2(500.f, 400.f));
    if (pMonster2) m_pScene->AddObject(pMonster2, GROUP_TYPE::MONSTER);

    // 플레이어 스폰 위치 설정
    m_pEditorCore->GetObjectManager()->SetPlayerSpawnPosition(Vec2(640.f, 400.f));
}

wstring CEditorFileManager::GetLevelDirectory()
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    return strContentPath + L"level\\";
}

wstring CEditorFileManager::GetFullLevelPath(const wstring& _strFileName)
{
    return GetLevelDirectory() + _strFileName + L".lvl";
}

bool CEditorFileManager::EnsureLevelDirectoryExists()
{
    wstring strLevelDir = GetLevelDirectory();
    return CreateDirectory(strLevelDir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

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

bool CEditorFileManager::IsValidVersion(int _iVersion)
{
    return _iVersion >= 1 && _iVersion <= 2;
}

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