#include "gamePCH.h"
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
#include "CMonster.h"
#include "CRigidBody.h"

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

// === 파일 처리 ===

void CEditorFileManager::SaveLevel(const wstring& _strFileName)
{
    OutputDebugStringW(L"[SAVE_DEBUG] SaveLevel started\n");
    
    // 레벨 데이터 생성
    tLevelData levelData = CreateLevelData(_strFileName);
    
    // WhispyWoods 저장 확인
    int whispyCount = 0;
    for (const auto& objData : levelData.vecObjects)
    {
        if (objData.eGroupType == GROUP_TYPE::MONSTER && 
            objData.iSubType == (int)OBJECT_TYPE::MONSTER_WHISPY_WOODS)
        {
            whispyCount++;
        }
    }

    // 파일 경로 생성
    wstring strFullPath = GetFullLevelPath(_strFileName);

    // 파일 디렉토리 확인
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
        // 파일 버전 (v1)
        int version = 1;
        fwrite(&version, sizeof(int), 1, pFile);

        // 레벨 이름 길이 및 이름
        size_t nameLen = levelData.strLevelName.length();
        fwrite(&nameLen, sizeof(size_t), 1, pFile);
        fwrite(levelData.strLevelName.c_str(), sizeof(wchar_t), nameLen, pFile);

        // 플레이어 스폰 위치
        fwrite(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // 배경 타입 저장
        fwrite(&levelData.iBackgroundType, sizeof(int), 1, pFile);

        // 레벨 범위 저장
        fwrite(&levelData.vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fwrite(&levelData.vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fwrite(&levelData.fGameOverY, sizeof(float), 1, pFile);

        // 스테이지 이미지 저장
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

        // 오브젝트 저장
        size_t objCount = levelData.vecObjects.size();
        fwrite(&objCount, sizeof(size_t), 1, pFile);

        // 각 오브젝트 저장하기
        for (const auto& objData : levelData.vecObjects)
        {
            fwrite(&objData, sizeof(tLevelObjectData), 1, pFile);
        }

        fclose(pFile);

        // 저장 메시지
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

        // ���� �̸� �ε�
        size_t nameLen;
        fread(&nameLen, sizeof(size_t), 1, pFile);
        if (nameLen > 0 && nameLen < 1000) // ������ �˻�
        {
            levelData.strLevelName.resize(nameLen);
            fread(&levelData.strLevelName[0], sizeof(wchar_t), nameLen, pFile);
        }

        // �÷��̾� ���� ��ġ
        fread(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

        // ��� Ÿ��
        fread(&levelData.iBackgroundType, sizeof(int), 1, pFile);

        // ��� ����
        fread(&levelData.vLevelBoundsMin, sizeof(Vec2), 1, pFile);
        fread(&levelData.vLevelBoundsMax, sizeof(Vec2), 1, pFile);
        fread(&levelData.fGameOverY, sizeof(float), 1, pFile);

        // �������� �̹��� Ÿ��
        int stageImageType;
        fread(&stageImageType, sizeof(int), 1, pFile);
        levelData.eStageType = (STAGE_IMAGE_TYPE)stageImageType;

        // �������� �̹��� ���
        size_t stagePathLen;
        fread(&stagePathLen, sizeof(size_t), 1, pFile);
        if (stagePathLen > 0 && stagePathLen < 1000) // ������ �˻�
        {
            levelData.strStageImagePath.resize(stagePathLen);
            fread(&levelData.strStageImagePath[0], sizeof(wchar_t), stagePathLen, pFile);
        }

        // �������� �̹��� ��ġ
        fread(&levelData.vStageImagePos, sizeof(Vec2), 1, pFile);

        // ��ü ����
        size_t objCount;
        fread(&objCount, sizeof(size_t), 1, pFile);
        
        levelData.vecObjects.reserve(objCount);
        int whispyWoodsLoadCount = 0;
        
        for (size_t i = 0; i < objCount; ++i)
        {
            tLevelObjectData objData;
            fread(&objData, sizeof(tLevelObjectData), 1, pFile);
            
            levelData.vecObjects.push_back(objData);
        }

        fclose(pFile);

        // �ε�� ������ ����
        ApplyLoadedLevelData(levelData);

        // ���� �޽���
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

    // �÷��̾� ���� ��ġ
    if (m_pEditorCore->GetObjectManager())
    {
        levelData.vPlayerSpawn = m_pEditorCore->GetObjectManager()->GetPlayerSpawnPos();
    }

    // ��� ����
    if (m_pEditorCore->GetObjectManager())
    {
        levelData.iBackgroundType = (int)m_pEditorCore->GetObjectManager()->GetCurrentBackgroundType();
    }

    // ��� ���� (���ο� �⺻��)
    levelData.vLevelBoundsMin = Vec2(0.f, 0.f);
    
    // 현재 에디터의 실제 맵 크기 사용
    Vec2 vCurrentMapSize = m_pEditorCore->GetMapSize();
    levelData.vLevelBoundsMax = vCurrentMapSize;
    levelData.fGameOverY = vCurrentMapSize.y + 40.f;  // 맵 하단에서 40px 아래

    // �������� �̹��� ���� (���� 1������ �⺻ ����������)
    levelData.eStageType = CStageMgr::GetInst()->GetCurrentStageType();

    // Ŀ���� �̹����� ��� ��� ����
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

    // �������� �̹��� ��ġ (�⺻��)
    levelData.vStageImagePos = Vec2(0.f, 0.f);

    // ��� �׷��� ��ü�� ����
    CollectSceneObjects(levelData);

    return levelData;
}

void CEditorFileManager::CollectSceneObjects(tLevelData& _levelData)
{
    if (!m_pScene)
        return;

    int whispyWoodsFound = 0;

    // �� �׷캰�� ��ü�� ����
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        GROUP_TYPE eGroupType = (GROUP_TYPE)i;

        // �÷��̾� �׷��� ���� (���� ��ġ�� ����)
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
    // �÷��̾� ���� ��ġ ����
    if (m_pEditorCore->GetObjectManager())
    {
        m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(_levelData.vPlayerSpawn);
    }

    // ��� ����
    if (m_pEditorCore->GetObjectManager())
    {
        BACKGROUND_TYPE bgType = (BACKGROUND_TYPE)_levelData.iBackgroundType;
        m_pEditorCore->GetObjectManager()->ChangeBackground(bgType);
    }

    // �������� �̹��� �ý��� ����
    if (_levelData.eStageType == STAGE_IMAGE_TYPE::CUSTOM && !_levelData.strStageImagePath.empty())
    {
        // Ŀ���� �̹��� �ε� (���� ����)
        // CStageMgr::GetInst()->LoadCustomStageImage(_levelData.strStageImagePath);
    }
    else
    {
        // �⺻ �������� �̹��� ����
        CStageMgr::GetInst()->SetCurrentStageImage(_levelData.eStageType);
    }

    for (const auto& objData : _levelData.vecObjects)
    {
        CObject* pObj = CreateObjectFromData(objData);
        if (pObj)
        {
            GROUP_TYPE eGroup = objData.eGroupType;
            m_pScene->AddObject(pObj, eGroup);
        }
    }

    // ��� ���� ����
    ApplyLevelBounds(_levelData);
}

// === 오브젝트 변환 ===

CObject* CEditorFileManager::CreateObjectFromData(const tLevelObjectData& _objData)
{
    CObject* pObj = nullptr;

    switch (_objData.eGroupType)
    {
    case GROUP_TYPE::MONSTER:
    {
        OBJECT_TYPE monsterType = (OBJECT_TYPE)_objData.iSubType;
        
        pObj = CObjectFactory::CreateObject(monsterType, _objData.vPos);
        
        // ������ ���� ���� ����
        if (pObj)
        {
            CMonster* pMonster = dynamic_cast<CMonster*>(pObj);
            if (pMonster)
            {
                pMonster->SetDirection(_objData.fDirection);
                
                // 에디터 모드로 설정하여 EDITOR_IDLE 상태 유지
                pMonster->SetEditorMode(true);
                pMonster->ChangeState(MONSTER_STATE::EDITOR_IDLE);
                
                // 중력 비활성화
                if (pMonster->GetRigidBody())
                {
                    pMonster->GetRigidBody()->SetUseGravity(false);
                    pMonster->GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
                }
            }
        }
    }
    break;

    case GROUP_TYPE::TILE:
    {
        OBJECT_TYPE tileType = (OBJECT_TYPE)_objData.iSubType;
        pObj = CObjectFactory::CreateObject(tileType, _objData.vPos);

        // Ÿ�� �ð� Ÿ�� �� �浹ü Ÿ�� ����
        if (pObj)
        {
            CTile* pTile = dynamic_cast<CTile*>(pObj);
            if (pTile)
            {
                // 타일 타입을 먼저 설정 (중요!)
                pTile->SetTileType((OBJECT_TYPE)_objData.iSubType);
                pTile->SetType((OBJECT_TYPE)_objData.iSubType);  // CObject의 기본 타입도 설정
                
                // �ð��� Ÿ�� ����
                if (_objData.iTileVisualType >= 0)
                {
                    pTile->SetVisualType((TILE_VISUAL_TYPE)_objData.iTileVisualType);
                }

                // �浹 Ÿ�� ����
                if (_objData.iCollisionType >= 0)
                {
                    pTile->SetCollisionType((COLLISION_TYPE)_objData.iCollisionType);
                }
                
                // 트리거 타일인 경우 카메라 좌표 복원
                if (pTile->GetTileType() == OBJECT_TYPE::TILE_TRIGGER)
                {
                    pTile->SetBossLockPosition(_objData.vBossLockPos);
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
    objData.iSubType = 0; // �⺻��

    // Ÿ���� ��� �ð� Ÿ�԰� �浹 Ÿ�� ������ ����
    if (objData.eGroupType == GROUP_TYPE::TILE)
    {
        CTile* pTile = dynamic_cast<CTile*>(_pObj);
        if (pTile)
        {
            objData.iSubType = (int)pTile->GetTileType();
            objData.iTileVisualType = (int)pTile->GetVisualType();
            objData.iCollisionType = (int)pTile->GetCollisionType();
            
            // 트리거 타일인 경우 카메라 좌표 저장
            if (pTile->GetTileType() == OBJECT_TYPE::TILE_TRIGGER)
            {
                objData.vBossLockPos = pTile->GetBossLockPosition();
            }
        }
    }
    // ������ ��� ���� Ÿ�� ���� ���� + ���� ����
    else if (objData.eGroupType == GROUP_TYPE::MONSTER)
    {
        objData.iSubType = (int)_pObj->GetType();

        // ������ ���� ���� ����
        CMonster* pMonster = dynamic_cast<CMonster*>(_pObj);
        if (pMonster)
        {
            objData.fDirection = pMonster->GetDirection();
        }
    }
    // �������� ��� ������ Ÿ�� ���� ����
    else if (objData.eGroupType == GROUP_TYPE::ITEM)
    {
        objData.iSubType = (int)_pObj->GetType();
    }
    // Ư�� ��ü�� ���
    else if (objData.eGroupType == GROUP_TYPE::SPECIAL)
    {
        objData.iSubType = (int)_pObj->GetType();
    }

    return objData;
}

// === 씬 관리 ===

void CEditorFileManager::ClearScene()
{
    // �÷��̾ ������ ��� ��ü ����
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
    // ���� ����
    m_pEditorCore->DeselectObject();
}

void CEditorFileManager::CreateDefaultLevel()
{
    // �⺻ ���� ���� (�� ����)
    ClearScene();

    // �÷��̾� ���� ��ġ �ʱ�ȭ (���ο� �� ũ�⿡ �°�)
    m_pEditorCore->GetObjectManager()->SetPlayerSpawnPos(Vec2(320.f, 320.f));

    // �⺻ ��� ����
    m_pEditorCore->GetObjectManager()->SetCurrentBackgroundType(BACKGROUND_TYPE::BACKGROUND1);

    // �⺻ ī�޶� ��� ���� (���ο� �� ũ��)
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
        // �ε�� ������ ��� ���� ����
        pCameraController->SetCameraBounds(_levelData.vLevelBoundsMin, _levelData.vLevelBoundsMax);

        // ī�޶� ���� ��� �� ������ ��ġ�� �̵�
        Vec2 vSafePos = Vec2(
            max(_levelData.vLevelBoundsMin.x + 480.f, 480.f), // ȭ�� ���� ����
            min(_levelData.vLevelBoundsMax.y - 320.f, 320.f)  // ȭ�� ���� ����
        );
        pCameraController->SetCameraPosition(vSafePos);
    }

    // ���ٿ� �� ũ�� ���� ������Ʈ
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

// === 파일 대화상자 처리 ===

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
    ofn.lpstrTitle = L"level file save";
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
    ofn.lpstrTitle = L"level file saved";
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