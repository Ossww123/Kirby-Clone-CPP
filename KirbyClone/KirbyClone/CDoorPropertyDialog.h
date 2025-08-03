#pragma once
#include "resource.h"

class CDoor;

// 문 속성 설정 다이얼로그
class CDoorPropertyDialog
{
private:
    HWND m_hDlg;
    CDoor* m_pTargetDoor;

    // 현재 설정값들
    SCENE_TYPE m_eTargetScene;
    Vec2 m_vTargetPosition;
    wstring m_strTargetDoorID;
    wstring m_strDoorID;
    bool m_bIsLocked;

public:
    // 다이얼로그 표시 및 결과 반환
    bool ShowDialog(HWND hParent, CDoor* pDoor);

    // 설정값 적용
    void ApplySettings();

private:
    // 다이얼로그 프로시저
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // 메시지 처리 함수들
    void OnInitDialog();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnOK();
    void OnCancel();

    // UI 업데이트 함수들
    void UpdateUI();
    void LoadSettings();
    void SaveSettings();

    // 씬 타입 변환 함수들
    void PopulateSceneComboBox();
    SCENE_TYPE GetSelectedSceneType();
    void SetSelectedSceneType(SCENE_TYPE eScene);

public:
    CDoorPropertyDialog();
    ~CDoorPropertyDialog();
};