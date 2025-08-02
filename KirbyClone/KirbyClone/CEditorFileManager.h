#pragma once

class CEditorCore;
class CScene;
class CObject;

class CEditorFileManager
{
private:
    CEditorCore* m_pEditorCore;
    CScene* m_pScene;

    // 빠른 저장/로드용 파일명
    wstring m_strQuickSaveFile;

public:
    void Initialize(CEditorCore* _pCore, CScene* _pScene);

    // 파일 작업
    void SaveAsDialog();
    void OpenDialog();
    void QuickSave();
    void QuickLoad();

    // 레벨 파일 처리
    void SaveLevel(const wstring& _strFileName);
    void LoadLevel(const wstring& _strFileName);

private:
    // 레벨 데이터 생성/처리
    tLevelData CreateLevelData(const wstring& _strLevelName);
    CObject* CreateObjectFromData(const tLevelObjectData& _objData);
    tLevelObjectData CreateObjectData(CObject* _pObj, GROUP_TYPE _eGroupType);

    // 씬 관리
    void ClearScene();
    void CreateDefaultLevel();

    // 파일 경로 유틸리티
    wstring GetLevelDirectory();
    wstring GetFullLevelPath(const wstring& _strFileName);
    bool EnsureLevelDirectoryExists();

    // 파일 대화상자 헬퍼
    bool ShowSaveDialog(wstring& _strFileName);
    bool ShowOpenDialog(wstring& _strFileName);

    // 파일 검증
    bool ValidateLevelFile(const wstring& _strFilePath);
    bool IsValidVersion(int _iVersion);

    // 에러 처리
    void ShowErrorMessage(const wstring& _strMessage);
    void ShowSuccessMessage(const wstring& _strMessage, int _iObjectCount = -1);

public:
    CEditorFileManager();
    ~CEditorFileManager();
};