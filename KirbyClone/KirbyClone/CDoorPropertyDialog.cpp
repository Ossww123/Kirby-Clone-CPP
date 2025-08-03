#include "pch.h"
#include "CDoorPropertyDialog.h"
#include "CDoor.h"
#include "resource.h"

CDoorPropertyDialog::CDoorPropertyDialog()
    : m_hDlg(nullptr)
    , m_pTargetDoor(nullptr)
    , m_eTargetScene(SCENE_TYPE::STAGE01)
    , m_vTargetPosition(Vec2(100.f, 400.f))
    , m_strTargetDoorID(L"")
    , m_strDoorID(L"door_default")
    , m_bIsLocked(false)
{
}

CDoorPropertyDialog::~CDoorPropertyDialog()
{
}

bool CDoorPropertyDialog::ShowDialog(HWND hParent, CDoor* pDoor)
{
    if (!pDoor)
        return false;

    m_pTargetDoor = pDoor;
    LoadSettings();

    // 다이얼로그 생성 및 표시
    INT_PTR result = DialogBoxParam(GetModuleHandle(nullptr),
        MAKEINTRESOURCE(IDD_DOOR_PROPERTIES),
        hParent,
        DlgProc,
        (LPARAM)this);

    return (result == IDOK);
}

void CDoorPropertyDialog::ApplySettings()
{
    if (!m_pTargetDoor)
        return;

    // 문 오브젝트에 설정 적용
    m_pTargetDoor->SetTargetScene(m_eTargetScene);
    m_pTargetDoor->SetTargetPosition(m_vTargetPosition);
    m_pTargetDoor->SetTargetDoorID(m_strTargetDoorID);
    m_pTargetDoor->SetDoorID(m_strDoorID);
    m_pTargetDoor->SetLocked(m_bIsLocked);
}

INT_PTR CALLBACK CDoorPropertyDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CDoorPropertyDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pThis = (CDoorPropertyDialog*)lParam;
        pThis->m_hDlg = hDlg;
        pThis->OnInitDialog();
        return TRUE;
    }
    else
    {
        pThis = (CDoorPropertyDialog*)GetWindowLongPtr(hDlg, DWLP_USER);
    }

    if (!pThis)
        return FALSE;

    switch (message)
    {
    case WM_COMMAND:
        pThis->OnCommand(wParam, lParam);
        return TRUE;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

void CDoorPropertyDialog::OnInitDialog()
{
    // 다이얼로그 제목 설정
    SetWindowText(m_hDlg, L"문 속성 설정");

    // 씬 콤보박스 채우기
    PopulateSceneComboBox();

    // 현재 설정값으로 UI 업데이트
    UpdateUI();
}

void CDoorPropertyDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDOK:
        OnOK();
        break;

    case IDCANCEL:
        OnCancel();
        break;

    case IDC_COMBO_TARGET_SCENE:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            SaveSettings();
        }
        break;

    case IDC_EDIT_TARGET_X:
    case IDC_EDIT_TARGET_Y:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            SaveSettings();
        }
        break;

    case IDC_EDIT_TARGET_DOOR_ID:
    case IDC_EDIT_DOOR_ID:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            SaveSettings();
        }
        break;

    case IDC_CHECK_LOCKED:
        SaveSettings();
        break;
    }
}

void CDoorPropertyDialog::OnOK()
{
    SaveSettings();
    ApplySettings();
    EndDialog(m_hDlg, IDOK);
}

void CDoorPropertyDialog::OnCancel()
{
    EndDialog(m_hDlg, IDCANCEL);
}

void CDoorPropertyDialog::UpdateUI()
{
    // 씬 타입 설정
    SetSelectedSceneType(m_eTargetScene);

    // 목표 위치 설정
    SetDlgItemText(m_hDlg, IDC_EDIT_TARGET_X, std::to_wstring((int)m_vTargetPosition.x).c_str());
    SetDlgItemText(m_hDlg, IDC_EDIT_TARGET_Y, std::to_wstring((int)m_vTargetPosition.y).c_str());

    // 문 ID 설정
    SetDlgItemText(m_hDlg, IDC_EDIT_TARGET_DOOR_ID, m_strTargetDoorID.c_str());
    SetDlgItemText(m_hDlg, IDC_EDIT_DOOR_ID, m_strDoorID.c_str());

    // 잠김 상태 설정
    CheckDlgButton(m_hDlg, IDC_CHECK_LOCKED, m_bIsLocked ? BST_CHECKED : BST_UNCHECKED);
}

void CDoorPropertyDialog::LoadSettings()
{
    if (!m_pTargetDoor)
        return;

    m_eTargetScene = m_pTargetDoor->GetTargetScene();
    m_vTargetPosition = m_pTargetDoor->GetTargetPosition();
    m_strTargetDoorID = m_pTargetDoor->GetTargetDoorID();
    m_strDoorID = m_pTargetDoor->GetDoorID();
    m_bIsLocked = m_pTargetDoor->IsLocked();
}

void CDoorPropertyDialog::SaveSettings()
{
    // 씬 타입 가져오기
    m_eTargetScene = GetSelectedSceneType();

    // 목표 위치 가져오기
    wchar_t szBuffer[64];
    GetDlgItemText(m_hDlg, IDC_EDIT_TARGET_X, szBuffer, 64);
    m_vTargetPosition.x = (float)_wtof(szBuffer);

    GetDlgItemText(m_hDlg, IDC_EDIT_TARGET_Y, szBuffer, 64);
    m_vTargetPosition.y = (float)_wtof(szBuffer);

    // 문 ID 가져오기
    GetDlgItemText(m_hDlg, IDC_EDIT_TARGET_DOOR_ID, szBuffer, 64);
    m_strTargetDoorID = szBuffer;

    GetDlgItemText(m_hDlg, IDC_EDIT_DOOR_ID, szBuffer, 64);
    m_strDoorID = szBuffer;

    // 잠김 상태 가져오기
    m_bIsLocked = (IsDlgButtonChecked(m_hDlg, IDC_CHECK_LOCKED) == BST_CHECKED);
}

void CDoorPropertyDialog::PopulateSceneComboBox()
{
    HWND hCombo = GetDlgItem(m_hDlg, IDC_COMBO_TARGET_SCENE);

    // 콤보박스 항목 추가
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Start Scene");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Stage 01");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Stage 02");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Tool Scene");

    // 기본 선택
    SendMessage(hCombo, CB_SETCURSEL, 1, 0); // Stage 01
}

SCENE_TYPE CDoorPropertyDialog::GetSelectedSceneType()
{
    HWND hCombo = GetDlgItem(m_hDlg, IDC_COMBO_TARGET_SCENE);
    int iSelected = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);

    switch (iSelected)
    {
    case 0: return SCENE_TYPE::START;
    case 1: return SCENE_TYPE::STAGE01;
    case 2: return SCENE_TYPE::STAGE02;
    case 3: return SCENE_TYPE::TOOL;
    default: return SCENE_TYPE::STAGE01;
    }
}

void CDoorPropertyDialog::SetSelectedSceneType(SCENE_TYPE eScene)
{
    HWND hCombo = GetDlgItem(m_hDlg, IDC_COMBO_TARGET_SCENE);

    int iIndex = 1; // 기본값: Stage 01
    switch (eScene)
    {
    case SCENE_TYPE::START:   iIndex = 0; break;
    case SCENE_TYPE::STAGE01: iIndex = 1; break;
    case SCENE_TYPE::STAGE02: iIndex = 2; break;
    case SCENE_TYPE::TOOL:    iIndex = 3; break;
    }

    SendMessage(hCombo, CB_SETCURSEL, iIndex, 0);
}