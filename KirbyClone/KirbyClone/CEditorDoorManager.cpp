#include "pch.h"
#include "CEditorDoorManager.h"
#include "CDoor.h"
#include "CDoorPropertyDialog.h"
#include "CCamera.h"

CEditorDoorManager::CEditorDoorManager()
    : m_pPropertyDialog(nullptr)
    , m_pSelectedDoor(nullptr)
{
    m_pPropertyDialog = new CDoorPropertyDialog;
}

CEditorDoorManager::~CEditorDoorManager()
{
    if (m_pPropertyDialog)
    {
        delete m_pPropertyDialog;
        m_pPropertyDialog = nullptr;
    }

    ClearAllDoors();
}

void CEditorDoorManager::AddDoor(CDoor* pDoor, const wstring& strSceneName)
{
    if (!pDoor)
        return;

    // 씬별 문 목록에 추가
    m_mapSceneDoors[strSceneName].push_back(pDoor);

    // 기본 문 ID 설정 (없는 경우)
    if (pDoor->GetDoorID().empty())
    {
        wstring strDoorID = strSceneName + L"_door_" + std::to_wstring(m_mapSceneDoors[strSceneName].size());
        pDoor->SetDoorID(strDoorID);
    }
}

void CEditorDoorManager::RemoveDoor(CDoor* pDoor, const wstring& strSceneName)
{
    if (!pDoor)
        return;

    auto& doorList = m_mapSceneDoors[strSceneName];
    doorList.erase(std::remove(doorList.begin(), doorList.end(), pDoor), doorList.end());

    // 선택된 문이었다면 선택 해제
    if (m_pSelectedDoor == pDoor)
    {
        m_pSelectedDoor = nullptr;
    }
}

void CEditorDoorManager::ClearAllDoors()
{
    m_mapSceneDoors.clear();
    m_pSelectedDoor = nullptr;
}

void CEditorDoorManager::SelectDoor(CDoor* pDoor)
{
    m_pSelectedDoor = pDoor;
}

bool CEditorDoorManager::ShowDoorProperties(HWND hParent)
{
    if (!m_pSelectedDoor || !m_pPropertyDialog)
        return false;

    return m_pPropertyDialog->ShowDialog(hParent, m_pSelectedDoor);
}

bool CEditorDoorManager::ValidateDoorConnections(const wstring& strSceneName)
{
    vector<wstring> errors = GetConnectionErrors(strSceneName);
    return errors.empty();
}

vector<wstring> CEditorDoorManager::GetConnectionErrors(const wstring& strSceneName)
{
    vector<wstring> errors;

    auto it = m_mapSceneDoors.find(strSceneName);
    if (it == m_mapSceneDoors.end())
        return errors;

    for (CDoor* pDoor : it->second)
    {
        // 문 ID 중복 검사
        wstring strDoorID = pDoor->GetDoorID();
        int count = 0;
        for (CDoor* pOtherDoor : it->second)
        {
            if (pOtherDoor->GetDoorID() == strDoorID)
                count++;
        }

        if (count > 1)
        {
            errors.push_back(L"중복된 문 ID: " + strDoorID);
        }

        // 타겟 씬 검증
        SCENE_TYPE eTargetScene = pDoor->GetTargetScene();
        wstring strTargetSceneName;

        switch (eTargetScene)
        {
        case SCENE_TYPE::START:   strTargetSceneName = L"START"; break;
        case SCENE_TYPE::STAGE01: strTargetSceneName = L"STAGE01"; break;
        case SCENE_TYPE::STAGE02: strTargetSceneName = L"STAGE02"; break;
        case SCENE_TYPE::TOOL:    strTargetSceneName = L"TOOL"; break;
        default:
            errors.push_back(L"잘못된 타겟 씬: " + strDoorID);
            continue;
        }

        // 타겟 문 ID 검증 (빈 값이 아닌 경우)
        wstring strTargetDoorID = pDoor->GetTargetDoorID();
        if (!strTargetDoorID.empty())
        {
            CDoor* pTargetDoor = FindDoorByID(strTargetSceneName, strTargetDoorID);
            if (!pTargetDoor)
            {
                errors.push_back(L"타겟 문을 찾을 수 없음: " + strDoorID + L" -> " + strTargetDoorID);
            }
        }
    }

    return errors;
}

vector<CDoor*> CEditorDoorManager::GetDoorsInScene(const wstring& strSceneName)
{
    auto it = m_mapSceneDoors.find(strSceneName);
    if (it != m_mapSceneDoors.end())
    {
        return it->second;
    }
    return vector<CDoor*>();
}

CDoor* CEditorDoorManager::FindDoorByID(const wstring& strSceneName, const wstring& strDoorID)
{
    auto it = m_mapSceneDoors.find(strSceneName);
    if (it == m_mapSceneDoors.end())
        return nullptr;

    for (CDoor* pDoor : it->second)
    {
        if (pDoor->GetDoorID() == strDoorID)
            return pDoor;
    }

    return nullptr;
}

void CEditorDoorManager::UpdateDoorList(HWND hListCtrl, const wstring& strSceneName)
{
    if (!hListCtrl)
        return;

    // 리스트 컨트롤 초기화
    ListView_DeleteAllItems(hListCtrl);

    auto it = m_mapSceneDoors.find(strSceneName);
    if (it == m_mapSceneDoors.end())
        return;

    // 문 목록 추가
    for (int i = 0; i < (int)it->second.size(); ++i)
    {
        CDoor* pDoor = it->second[i];

        // 문 ID
        LVITEM item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = (LPWSTR)pDoor->GetDoorID().c_str();
        item.lParam = (LPARAM)pDoor;
        ListView_InsertItem(hListCtrl, &item);

        // 타겟 씬
        wstring strTargetScene;
        switch (pDoor->GetTargetScene())
        {
        case SCENE_TYPE::START:   strTargetScene = L"START"; break;
        case SCENE_TYPE::STAGE01: strTargetScene = L"STAGE01"; break;
        case SCENE_TYPE::STAGE02: strTargetScene = L"STAGE02"; break;
        case SCENE_TYPE::TOOL:    strTargetScene = L"TOOL"; break;
        default: strTargetScene = L"UNKNOWN"; break;
        }
        ListView_SetItemText(hListCtrl, i, 1, (LPWSTR)strTargetScene.c_str());

        // 타겟 위치
        Vec2 vTargetPos = pDoor->GetTargetPosition();
        wstring strTargetPos = L"(" + std::to_wstring((int)vTargetPos.x) + L", " + std::to_wstring((int)vTargetPos.y) + L")";
        ListView_SetItemText(hListCtrl, i, 2, (LPWSTR)strTargetPos.c_str());

        // 타겟 문 ID
        ListView_SetItemText(hListCtrl, i, 3, (LPWSTR)pDoor->GetTargetDoorID().c_str());

        // 잠금 상태
        wstring strLocked = pDoor->IsLocked() ? L"잠김" : L"열림";
        ListView_SetItemText(hListCtrl, i, 4, (LPWSTR)strLocked.c_str());
    }
}

void CEditorDoorManager::RenderDoorConnections(HDC _dc)
{
    // 문 연결 관계를 시각적으로 표시
    // 현재 씬의 문들과 연결선 그리기

    for (auto& scenePair : m_mapSceneDoors)
    {
        for (CDoor* pDoor : scenePair.second)
        {
            Vec2 vDoorPos = CCamera::GetInst()->GetRenderPos(pDoor->GetPos());

            // 문 위에 ID 표시
            SetTextColor(_dc, RGB(255, 255, 0));
            SetBkMode(_dc, TRANSPARENT);

            RECT textRect;
            textRect.left = (int)(vDoorPos.x - 50);
            textRect.top = (int)(vDoorPos.y - pDoor->GetScale().y / 2 - 20);
            textRect.right = (int)(vDoorPos.x + 50);
            textRect.bottom = (int)(vDoorPos.y - pDoor->GetScale().y / 2);

            DrawText(_dc, pDoor->GetDoorID().c_str(), -1, &textRect, DT_CENTER | DT_VCENTER);

            // 연결선 그리기 (같은 씬 내의 문들만)
            wstring strTargetDoorID = pDoor->GetTargetDoorID();
            if (!strTargetDoorID.empty())
            {
                CDoor* pTargetDoor = FindDoorByID(scenePair.first, strTargetDoorID);
                if (pTargetDoor)
                {
                    Vec2 vTargetPos = CCamera::GetInst()->GetRenderPos(pTargetDoor->GetPos());

                    HPEN hPen = CreatePen(PS_DASH, 2, RGB(0, 255, 0));
                    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

                    MoveToEx(_dc, (int)vDoorPos.x, (int)vDoorPos.y, nullptr);
                    LineTo(_dc, (int)vTargetPos.x, (int)vTargetPos.y);

                    SelectObject(_dc, hOldPen);
                    DeleteObject(hPen);
                }
            }
        }
    }
}

void CEditorDoorManager::AutoConnectDoors(const wstring& strScene1, const wstring& strScene2)
{
    vector<CDoor*> doors1 = GetDoorsInScene(strScene1);
    vector<CDoor*> doors2 = GetDoorsInScene(strScene2);

    // 각 씬에 문이 하나씩만 있는 경우 자동 연결
    if (doors1.size() == 1 && doors2.size() == 1)
    {
        CDoor* pDoor1 = doors1[0];
        CDoor* pDoor2 = doors2[0];

        // Scene1의 문이 Scene2로 이동하도록 설정
        SCENE_TYPE eTargetScene2 = SCENE_TYPE::STAGE01;
        if (strScene2 == L"STAGE01") eTargetScene2 = SCENE_TYPE::STAGE01;
        else if (strScene2 == L"STAGE02") eTargetScene2 = SCENE_TYPE::STAGE02;
        else if (strScene2 == L"START") eTargetScene2 = SCENE_TYPE::START;

        pDoor1->SetTargetScene(eTargetScene2);
        pDoor1->SetTargetPosition(pDoor2->GetPos());
        pDoor1->SetTargetDoorID(pDoor2->GetDoorID());

        // Scene2의 문이 Scene1로 이동하도록 설정
        SCENE_TYPE eTargetScene1 = SCENE_TYPE::STAGE01;
        if (strScene1 == L"STAGE01") eTargetScene1 = SCENE_TYPE::STAGE01;
        else if (strScene1 == L"STAGE02") eTargetScene1 = SCENE_TYPE::STAGE02;
        else if (strScene1 == L"START") eTargetScene1 = SCENE_TYPE::START;

        pDoor2->SetTargetScene(eTargetScene1);
        pDoor2->SetTargetPosition(pDoor1->GetPos());
        pDoor2->SetTargetDoorID(pDoor1->GetDoorID());
    }
}

void CEditorDoorManager::SuggestDoorConnections(const wstring& strSceneName)
{
    // 연결되지 않은 문들을 찾아서 자동 연결 제안
    vector<CDoor*> doors = GetDoorsInScene(strSceneName);

    for (CDoor* pDoor : doors)
    {
        if (pDoor->GetTargetDoorID().empty())
        {
            // 다른 씬의 문들과 자동 매칭 시도
            for (auto& scenePair : m_mapSceneDoors)
            {
                if (scenePair.first == strSceneName)
                    continue;

                for (CDoor* pOtherDoor : scenePair.second)
                {
                    if (pOtherDoor->GetTargetDoorID().empty())
                    {
                        // 서로 연결 제안
                        // 실제 구현에서는 UI를 통해 사용자에게 확인받아야 함
                        break;
                    }
                }
            }
        }
    }
}