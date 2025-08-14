#pragma once

class CEditorCore;
class CScene;
class CObject;

class CEditorFileManager
{
public:
    // === 핵심 생명주기 함수 ===
    CEditorFileManager();
    ~CEditorFileManager();

    void Initialize(CEditorCore* _pCore, CScene* _pScene);

public:
    // === 파일 저장/로드 인터페이스 ===
    void SaveAsDialog();
    void OpenDialog();
    void QuickSave();
    void QuickLoad();

    // === 레벨 파일 처리 ===
    void SaveLevel(const wstring& _strFileName);
    void LoadLevel(const wstring& _strFileName);

private:
    // === 레벨 데이터 생성/처리 ===
    tLevelData CreateLevelData(const wstring& _strLevelName);
    void CollectSceneObjects(tLevelData& _levelData);
    void ApplyLoadedLevelData(const tLevelData& _levelData);

    // === 오브젝트 변환 ===
    CObject* CreateObjectFromData(const tLevelObjectData& _objData);
    tLevelObjectData CreateObjectData(CObject* _pObj, GROUP_TYPE _eGroupType);

    // === 씬 관리 ===
    void ClearScene();
    void CreateDefaultLevel();
    void ApplyLevelBounds(const tLevelData& _levelData);

private:
    // === 파일 경로 유틸리티 ===
    wstring GetLevelDirectory() const;
    wstring GetFullLevelPath(const wstring& _strFileName) const;
    bool EnsureLevelDirectoryExists();

    // === 파일 대화상자 헬퍼 ===
    bool ShowSaveDialog(wstring& _strFileName);
    bool ShowOpenDialog(wstring& _strFileName);

    // === 파일 검증 ===
    bool ValidateLevelFile(const wstring& _strFilePath);
    bool IsValidVersion(int _iVersion) const;

    // === 메시지 시스템 ===
    void ShowErrorMessage(const wstring& _strMessage);
    void ShowSuccessMessage(const wstring& _strMessage, int _iObjectCount = -1);

private:
    // === 에디터 시스템 참조 ===
    CEditorCore* m_pEditorCore;
    CScene* m_pScene;

    // === 빠른 저장/로드 설정 ===
    wstring             m_strQuickSaveFile;
};