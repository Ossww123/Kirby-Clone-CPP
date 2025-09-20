#pragma once

class CDoor;
class CDoorPropertyDialog;

// 레벨 에디터에서 문 오브젝트를 관리하는 클래스
class CEditorDoorManager
{
private:
    CDoorPropertyDialog* m_pPropertyDialog;   // 문 속성 다이얼로그
    CDoor* m_pSelectedDoor;                   // 현재 선택된 문

    // 문 오브젝트 목록 (씬별 관리)
    map<wstring, vector<CDoor*>> m_mapSceneDoors;

public:
    // 문 오브젝트 관리
    void AddDoor(CDoor* pDoor, const wstring& strSceneName);
    void RemoveDoor(CDoor* pDoor, const wstring& strSceneName);
    void ClearAllDoors();

    // 문 선택 및 편집
    void SelectDoor(CDoor* pDoor);
    bool ShowDoorProperties(HWND hParent);

    // 문 연결 검증
    bool ValidateDoorConnections(const wstring& strSceneName);
    vector<wstring> GetConnectionErrors(const wstring& strSceneName);

    // 문 목록 조회
    vector<CDoor*> GetDoorsInScene(const wstring& strSceneName);
    CDoor* FindDoorByID(const wstring& strSceneName, const wstring& strDoorID);

    // 에디터 UI 업데이트
    void UpdateDoorList(HWND hListCtrl, const wstring& strSceneName);
    void RenderDoorConnections(HDC _dc);  // 문 연결 시각화

    // 자동 연결 기능
    void AutoConnectDoors(const wstring& strScene1, const wstring& strScene2);
    void SuggestDoorConnections(const wstring& strSceneName);

public:
    CEditorDoorManager();
    ~CEditorDoorManager();
};