#include "pch.h"
#include "CScene_Tool.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"
#include "CKeyMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CTimeMgr.h"
#include "CEventMgr.h"
#include "CPathMgr.h"

CScene_Tool::CScene_Tool()
    : m_bShowUI(true)
    , m_vMousePos{}
    , m_bMouseClick(false)
    , m_fClickTime(0.f)
    , m_eCurrentMode(EDITOR_MODE::NONE)
    , m_pSelectedObject(nullptr)
    , m_bDragging(false)
    , m_vDragStartPos{}
{
}

CScene_Tool::~CScene_Tool()
{
}

void CScene_Tool::Enter()
{
    // 기존 테스트 오브젝트들 그대로 유지
    // 몬스터 여러 마리 배치 테스트
    for (int i = 0; i < 5; ++i)
    {
        CMonster* pMonster = new CMonster;
        pMonster->SetPos(Vec2(200.f + i * 200.f, 300.f + i * 50.f));
        pMonster->SetScale(Vec2(50.f, 50.f));
        AddObject(pMonster, GROUP_TYPE::MONSTER);
    }

    // 플레이어도 하나 추가 (테스트용)
    CPlayer* pPlayer = new CPlayer;
    pPlayer->SetPos(Vec2(640.f, 600.f));
    pPlayer->SetScale(Vec2(100.f, 100.f));
    AddObject(pPlayer, GROUP_TYPE::PLAYER);

    // 에디터 모드 안내 메시지
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor Mode - Mouse input enabled");
}

void CScene_Tool::Exit()
{
    DeleteAllObject();  // 부모 클래스의 함수 호출
}

void CScene_Tool::Update()
{
    // 부모 클래스의 Update 호출 (모든 오브젝트 업데이트)
    CScene::Update();

    // 카메라 이동 처리 (항상 활성화)
    UpdateCameraMove();

    // 에디터 전용 입력 처리
    UpdateInput();
    UpdateMouse();
}

void CScene_Tool::Render(HDC _dc)
{
    // 부모 클래스의 Render 호출 (모든 오브젝트 렌더링)
    CScene::Render(_dc);

    // 선택된 오브젝트 하이라이트 (오브젝트 위에)
    RenderSelectedObject(_dc);

    // 배치 미리보기 렌더링 (마우스 커서보다 먼저)
    RenderPreview(_dc);

    // 마우스 커서 렌더링
    RenderMouse(_dc);

    // 에디터 UI 렌더링 (맨 위에 그려야 함)
    if (m_bShowUI)
    {
        RenderUI(_dc);
    }
}

void CScene_Tool::UpdateInput()
{
    // UI 토글 (H 키)
    if (KEY_TAP(KEY::H))
    {
        m_bShowUI = !m_bShowUI;

        if (m_bShowUI)
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - UI ON");
        else
            SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Level Editor - UI OFF");
    }

    // 모드 전환 입력 처리
    UpdateModeInput();
}

void CScene_Tool::UpdateMouse()
{
    // CKeyMgr에서 마우스 좌표 가져오기
    m_vMousePos = CKeyMgr::GetInst()->GetMouseWorldPos();

    // 마우스 클릭 감지 (새로운 방식 사용)
    if (KEY_TAP(KEY::MOUSE_LEFT))
    {
        m_bMouseClick = true;
        m_fClickTime = 0.5f;  // 0.5초 동안 클릭 표시

        // 현재 모드에 따른 동작 처리
        HandleMouseClick();
    }

    // 마우스 버튼을 떼면 드래그 종료
    if (KEY_AWAY(KEY::MOUSE_LEFT))
    {
        m_bDragging = false;
    }

    // 드래그 중이면 선택된 오브젝트 이동
    if (m_bDragging && m_pSelectedObject && m_eCurrentMode == EDITOR_MODE::SELECT)
    {
        m_pSelectedObject->SetPos(m_vMousePos);
    }

    // 클릭 표시 시간 감소
    if (m_fClickTime > 0.f)
    {
        m_fClickTime -= CTimeMgr::GetInst()->GetfDT();
        if (m_fClickTime <= 0.f)
        {
            m_bMouseClick = false;
        }
    }
}

void CScene_Tool::RenderMouse(HDC _dc)
{
    // 마우스 월드 좌표를 화면 좌표로 변환
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vMousePos);

    // 모드에 따른 커서 색상 설정
    COLORREF cursorColor = RGB(255, 255, 0); // 기본: 노란색
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
        cursorColor = RGB(100, 255, 100); // 녹색
        break;
    case EDITOR_MODE::SELECT:
        cursorColor = RGB(100, 200, 255); // 파란색
        break;
    case EDITOR_MODE::ERASE:
        cursorColor = RGB(255, 100, 100); // 빨간색
        break;
    case EDITOR_MODE::CAMERA_MOVE:
        cursorColor = RGB(255, 255, 100); // 노란색
        break;
    }

    // 마우스 커서 그리기 (십자가)
    HPEN hPen = CreatePen(PS_SOLID, 2, cursorColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    int size = 8;
    // 가로선
    MoveToEx(_dc, (int)vRenderPos.x - size, (int)vRenderPos.y, nullptr);
    LineTo(_dc, (int)vRenderPos.x + size, (int)vRenderPos.y);
    // 세로선
    MoveToEx(_dc, (int)vRenderPos.x, (int)vRenderPos.y - size, nullptr);
    LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y + size);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);

    // 클릭했을 때 원 그리기 (색상은 모드에 따라)
    if (m_bMouseClick)
    {
        HPEN hClickPen = CreatePen(PS_SOLID, 3, cursorColor);
        HPEN hOldClickPen = (HPEN)SelectObject(_dc, hClickPen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

        Ellipse(_dc,
            (int)vRenderPos.x - 15, (int)vRenderPos.y - 15,
            (int)vRenderPos.x + 15, (int)vRenderPos.y + 15);

        SelectObject(_dc, hOldClickPen);
        SelectObject(_dc, hOldBrush);
        DeleteObject(hClickPen);
    }
}

void CScene_Tool::RenderUI(HDC _dc)
{
    // UI 배경 패널 (크기 증가)
    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));  // 어두운 회색
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 패널 크기
    Rectangle(_dc, 10, 10, 380, 340);

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 테두리
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);

    Rectangle(_dc, 10, 10, 380, 340);

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush2);
    DeleteObject(hPen);

    // 텍스트 설정
    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    // UI 텍스트 정보
    int yPos = 20;
    int lineHeight = 18;

    // 제목
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    TextOut(_dc, 20, yPos, L"=== LEVEL EDITOR ===", 21);

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);

    // 일반 폰트로 변경
    yPos += 25;

    // 현재 모드 표시 (강조)
    wchar_t szBuffer[256];
    SetTextColor(_dc, RGB(100, 255, 100)); // 녹색으로 강조
    swprintf_s(szBuffer, L"Mode: %s", GetModeString());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    SetTextColor(_dc, RGB(255, 255, 255)); // 다시 흰색으로
    yPos += lineHeight + 5;

    // 현재 오브젝트 개수 정보
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    const vector<CObject*>& vecMonster = GetGroupObject(GROUP_TYPE::MONSTER);

    swprintf_s(szBuffer, L"Players: %d", (int)vecPlayer.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    swprintf_s(szBuffer, L"Monsters: %d", (int)vecMonster.size());
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    // 선택된 오브젝트 정보 추가
    if (m_pSelectedObject)
    {
        yPos += 5;
        SetTextColor(_dc, RGB(255, 255, 100)); // 노란색으로 강조
        TextOut(_dc, 20, yPos, L"Selected Object:", 16);
        yPos += lineHeight;

        Vec2 vSelPos = m_pSelectedObject->GetPos();
        Vec2 vSelScale = m_pSelectedObject->GetScale();

        swprintf_s(szBuffer, L"Pos: (%.0f, %.0f)", vSelPos.x, vSelPos.y);
        TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
        yPos += lineHeight;

        swprintf_s(szBuffer, L"Size: (%.0f, %.0f)", vSelScale.x, vSelScale.y);
        TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
        yPos += lineHeight;

        if (m_bDragging)
        {
            SetTextColor(_dc, RGB(100, 255, 100)); // 녹색
            TextOut(_dc, 20, yPos, L"Dragging...", 11);
            yPos += lineHeight;
        }

        SetTextColor(_dc, RGB(255, 255, 255)); // 다시 흰색으로
        yPos += 5;
    }

    // 마우스 정보
    yPos += 5;
    swprintf_s(szBuffer, L"Mouse: (%.0f, %.0f)", m_vMousePos.x, m_vMousePos.y);
    TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
    yPos += lineHeight;

    // 클릭 상태 표시 (시간 기반으로 개선)
    if (m_bMouseClick && m_fClickTime > 0.f)
    {
        SetTextColor(_dc, RGB(255, 100, 100)); // 빨간색으로 변경
        swprintf_s(szBuffer, L"CLICK! (%.1f)", m_fClickTime);
        TextOut(_dc, 20, yPos, szBuffer, wcslen(szBuffer));
        SetTextColor(_dc, RGB(255, 255, 255)); // 다시 흰색으로
    }
    else
    {
        // 현재 모드에 따른 액션 안내
        switch (m_eCurrentMode)
        {
        case EDITOR_MODE::PLACE_MONSTER:
            SetTextColor(_dc, RGB(100, 255, 100)); // 녹색
            TextOut(_dc, 20, yPos, L"Click to place monster", 22);
            break;
        case EDITOR_MODE::SELECT:
            SetTextColor(_dc, RGB(100, 200, 255)); // 파란색
            TextOut(_dc, 20, yPos, L"Click to select object", 22);
            break;
        case EDITOR_MODE::ERASE:
            SetTextColor(_dc, RGB(255, 150, 100)); // 주황색
            TextOut(_dc, 20, yPos, L"Click to delete object", 23);
            break;
        default:
            TextOut(_dc, 20, yPos, L"Ready to click...", 17);
            break;
        }
        SetTextColor(_dc, RGB(255, 255, 255)); // 다시 흰색으로
    }
    yPos += lineHeight;

    // 구분선
    yPos += 5;
    HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN hOldLinePen = (HPEN)SelectObject(_dc, hLinePen);

    MoveToEx(_dc, 20, yPos, nullptr);
    LineTo(_dc, 360, yPos);

    SelectObject(_dc, hOldLinePen);
    DeleteObject(hLinePen);
    yPos += 10;

    // 컨트롤 안내
    TextOut(_dc, 20, yPos, L"Mode Controls:", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Q - Monster Place Mode", 22);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"W - Select Mode", 15);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"E - Erase Mode", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"ESC - Normal Mode", 17);
    yPos += lineHeight;

    // 기타 컨트롤
    yPos += 5;
    TextOut(_dc, 20, yPos, L"Other Controls:", 15);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"Arrow Keys - Move Camera", 24);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"F - Quick Save", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"V - Quick Load", 14);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"H - Toggle UI", 13);
    yPos += lineHeight;
    TextOut(_dc, 20, yPos, L"T - Return to Game", 18);
}

void CScene_Tool::UpdateModeInput()
{
    // Q키: 몬스터 배치 모드
    if (KEY_TAP(KEY::Q))
    {
        ChangeMode(EDITOR_MODE::PLACE_MONSTER);
    }
    // W키: 선택 모드
    else if (KEY_TAP(KEY::W))
    {
        ChangeMode(EDITOR_MODE::SELECT);
    }
    // E키: 삭제 모드
    else if (KEY_TAP(KEY::E))
    {
        ChangeMode(EDITOR_MODE::ERASE);
    }
    // R키: 카메라 이동 모드 (추후 구현)
    else if (KEY_TAP(KEY::R))
    {
        ChangeMode(EDITOR_MODE::CAMERA_MOVE);
    }
    // ESC키: 기본 모드로 돌아가기
    else if (KEY_TAP(KEY::ESC))
    {
        ChangeMode(EDITOR_MODE::NONE);
    }
    else if (KEY_TAP(KEY::F))
    {
        QuickSave();  // ← 여기서 F키로 저장!
    }
    // V키: 빠른 로딩 (F9 대신 V키 사용)  
    else if (KEY_TAP(KEY::V))
    {
        QuickLoad();
    }
}

void CScene_Tool::ChangeMode(EDITOR_MODE _eMode)
{
    m_eCurrentMode = _eMode;

    // 모드 변경 시 윈도우 타이틀 업데이트
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Level Editor - Mode: %s", GetModeString());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

const wchar_t* CScene_Tool::GetModeString()
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::NONE:         return L"Normal";
    case EDITOR_MODE::PLACE_MONSTER: return L"Place Monster";
    case EDITOR_MODE::SELECT:       return L"Select";
    case EDITOR_MODE::ERASE:        return L"Erase";
    case EDITOR_MODE::CAMERA_MOVE:  return L"Camera Move";
    default:                        return L"Unknown";
    }
}

void CScene_Tool::HandleMouseClick()
{
    switch (m_eCurrentMode)
    {
    case EDITOR_MODE::PLACE_MONSTER:
        PlaceMonster(m_vMousePos);
        break;

    case EDITOR_MODE::SELECT:
    {
        // 클릭한 위치에서 오브젝트 찾기
        CObject* pClickedObj = FindObjectAtPosition(m_vMousePos);
        if (pClickedObj)
        {
            SetSelectedObject(pClickedObj);
            m_bDragging = true;
            m_vDragStartPos = m_vMousePos;
        }
        else
        {
            // 빈 공간 클릭 시 선택 해제
            DeselectObject();
        }
    }
    break;

    case EDITOR_MODE::ERASE:
        DeleteObjectAtPosition(m_vMousePos);
        break;

    case EDITOR_MODE::NONE:
    case EDITOR_MODE::CAMERA_MOVE:
    default:
        // 기본 모드에서는 클릭 위치만 표시
        {
            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Clicked at: (%.0f, %.0f)", m_vMousePos.x, m_vMousePos.y);
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
        }
        break;
    }
}

void CScene_Tool::PlaceMonster(Vec2 _vPos)
{
    // 새 몬스터 생성
    CMonster* pMonster = new CMonster;
    pMonster->SetPos(_vPos);
    pMonster->SetScale(Vec2(50.f, 50.f));  // 기본 크기

    // 씬에 추가
    AddObject(pMonster, GROUP_TYPE::MONSTER);

    // 성공 메시지 표시
    const vector<CObject*>& vecMonster = GetGroupObject(GROUP_TYPE::MONSTER);
    wchar_t szBuffer[256];
    swprintf_s(szBuffer, L"Monster placed! Total: %d monsters", (int)vecMonster.size());
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
}

void CScene_Tool::RenderPreview(HDC _dc)
{
    // 배치 모드일 때 배치 미리보기
    if (m_eCurrentMode == EDITOR_MODE::PLACE_MONSTER)
    {
        // 마우스 월드 좌표를 화면 좌표로 변환
        Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_vMousePos);

        // 배치될 몬스터 크기 (실제 몬스터와 같은 크기)
        Vec2 vMonsterSize = Vec2(50.f, 50.f);

        // 반투명 효과를 위한 펜과 브러쉬 설정
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 255, 100)); // 연한 녹색 테두리
        HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

        // 반투명 브러쉬 (Windows에서는 완전한 반투명이 어려우므로 점선 패턴 사용)
        HBRUSH hBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(100, 255, 100)); // 대각선 패턴
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

        // 미리보기 사각형 그리기
        Rectangle(_dc,
            (int)(vRenderPos.x - vMonsterSize.x / 2.f),
            (int)(vRenderPos.y - vMonsterSize.y / 2.f),
            (int)(vRenderPos.x + vMonsterSize.x / 2.f),
            (int)(vRenderPos.y + vMonsterSize.y / 2.f));

        // 중앙에 작은 원 표시 (몬스터임을 나타냄)
        Ellipse(_dc,
            (int)vRenderPos.x - 8, (int)vRenderPos.y - 8,
            (int)vRenderPos.x + 8, (int)vRenderPos.y + 8);

        // 원래 펜과 브러쉬 복원
        SelectObject(_dc, hOldPen);
        SelectObject(_dc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        // 미리보기 텍스트 표시
        SetTextColor(_dc, RGB(100, 255, 100));
        SetBkMode(_dc, TRANSPARENT);

        // 폰트 설정
        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

        // 텍스트 위치 (미리보기 박스 위쪽)
        int textX = (int)vRenderPos.x - 25;
        int textY = (int)vRenderPos.y - (int)vMonsterSize.y / 2 - 20;

        TextOut(_dc, textX, textY, L"Monster", 7);

        // 폰트 복원
        SelectObject(_dc, hOldFont);
        DeleteObject(hFont);
    }
    // 삭제 모드일 때 삭제 대상 표시
    else if (m_eCurrentMode == EDITOR_MODE::ERASE)
    {
        CObject* pTargetObj = FindObjectAtPosition(m_vMousePos);
        if (pTargetObj)
        {
            Vec2 vPos = pTargetObj->GetPos();
            Vec2 vScale = pTargetObj->GetScale();
            Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

            // 삭제 대상 표시 (빨간색 X표시)
            HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 100, 100)); // 빨간색
            HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

            // X 표시 그리기
            int halfSize = (int)(max(vScale.x, vScale.y) / 2.f + 10);
            MoveToEx(_dc, (int)vRenderPos.x - halfSize, (int)vRenderPos.y - halfSize, nullptr);
            LineTo(_dc, (int)vRenderPos.x + halfSize, (int)vRenderPos.y + halfSize);
            MoveToEx(_dc, (int)vRenderPos.x + halfSize, (int)vRenderPos.y - halfSize, nullptr);
            LineTo(_dc, (int)vRenderPos.x - halfSize, (int)vRenderPos.y + halfSize);

            SelectObject(_dc, hOldPen);
            DeleteObject(hPen);

            // "DELETE" 텍스트 표시
            SetTextColor(_dc, RGB(255, 100, 100));
            SetBkMode(_dc, TRANSPARENT);

            HFONT hFont = CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

            int textX = (int)vRenderPos.x - 20;
            int textY = (int)vRenderPos.y - halfSize - 20;

            TextOut(_dc, textX, textY, L"DELETE", 6);

            SelectObject(_dc, hOldFont);
            DeleteObject(hFont);
        }
    }
}

void CScene_Tool::UpdateCameraMove()
{
    // 카메라 이동 (화살표 키 사용)
    float fCameraSpeed = 500.f * CTimeMgr::GetInst()->GetfDT();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    if (KEY_HOLD(KEY::UP))      // W → UP
        vCameraPos.y -= fCameraSpeed;
    if (KEY_HOLD(KEY::DOWN))    // S → DOWN  
        vCameraPos.y += fCameraSpeed;
    if (KEY_HOLD(KEY::LEFT))    // A → LEFT
        vCameraPos.x -= fCameraSpeed;
    if (KEY_HOLD(KEY::RIGHT))   // D → RIGHT
        vCameraPos.x += fCameraSpeed;

    CCamera::GetInst()->SetLookAt(vCameraPos);
}

CObject* CScene_Tool::FindObjectAtPosition(Vec2 _vPos)
{
    // 모든 그룹에서 오브젝트 검색 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = GetGroupObject((GROUP_TYPE)i);

        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            Vec2 vObjPos = vecObj[j]->GetPos();
            Vec2 vObjScale = vecObj[j]->GetScale();

            // AABB 검사 (사각형 충돌 검사)
            if (_vPos.x >= vObjPos.x - vObjScale.x / 2.f &&
                _vPos.x <= vObjPos.x + vObjScale.x / 2.f &&
                _vPos.y >= vObjPos.y - vObjScale.y / 2.f &&
                _vPos.y <= vObjPos.y + vObjScale.y / 2.f)
            {
                return vecObj[j];
            }
        }
    }

    return nullptr;
}

void CScene_Tool::SetSelectedObject(CObject* _pObj)  // 함수 이름 변경
{
    m_pSelectedObject = _pObj;

    if (_pObj)
    {
        Vec2 vPos = _pObj->GetPos();
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"Selected object at (%.0f, %.0f)", vPos.x, vPos.y);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
    }
}

void CScene_Tool::DeselectObject()
{
    m_pSelectedObject = nullptr;
    SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Object deselected");
}

void CScene_Tool::DeleteObjectAtPosition(Vec2 _vPos)
{
    // 클릭한 위치에서 오브젝트 찾기
    CObject* pTargetObj = FindObjectAtPosition(_vPos);

    if (!pTargetObj)
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"No object to delete");
        return;
    }

    // 선택된 오브젝트가 삭제 대상이라면 선택 해제
    if (m_pSelectedObject == pTargetObj)
    {
        m_pSelectedObject = nullptr;
    }

    // 벡터에서 직접 제거
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(GetGroupObject((GROUP_TYPE)i));

        auto iter = find(vecObj.begin(), vecObj.end(), pTargetObj);
        if (iter != vecObj.end())
        {
            delete pTargetObj;  // 메모리 해제
            vecObj.erase(iter); // 벡터에서 제거

            wchar_t szBuffer[256];
            swprintf_s(szBuffer, L"Object deleted successfully! Remaining: %d", (int)vecObj.size());
            SetWindowText(CCore::GetInst()->GetMainHwnd(), szBuffer);
            return;
        }
    }
}

void CScene_Tool::RenderSelectedObject(HDC _dc)
{
    if (!m_pSelectedObject)
        return;

    Vec2 vPos = m_pSelectedObject->GetPos();
    Vec2 vScale = m_pSelectedObject->GetScale();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 선택 표시 (노란색 테두리)
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0)); // 노란색
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 선택 사각형 (약간 더 크게)
    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f - 3),
        (int)(vRenderPos.y - vScale.y / 2.f - 3),
        (int)(vRenderPos.x + vScale.x / 2.f + 3),
        (int)(vRenderPos.y + vScale.y / 2.f + 3));

    // 드래그 중일 때 추가 표시
    if (m_bDragging)
    {
        // 점선으로 이동 경로 표시
        HPEN hDragPen = CreatePen(PS_DOT, 1, RGB(255, 255, 100));
        HPEN hOldDragPen = (HPEN)SelectObject(_dc, hDragPen);

        Vec2 vDragStartRender = CCamera::GetInst()->GetRenderPos(m_vDragStartPos);
        MoveToEx(_dc, (int)vDragStartRender.x, (int)vDragStartRender.y, nullptr);
        LineTo(_dc, (int)vRenderPos.x, (int)vRenderPos.y);

        SelectObject(_dc, hOldDragPen);
        DeleteObject(hDragPen);
    }

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);
}

void CScene_Tool::SaveLevel(const wstring& _strFileName)
{
    // 레벨 데이터 수집
    tLevelData levelData;
    levelData.strLevelName = _strFileName;
    levelData.iVersion = 1;

    // 플레이어 스폰 위치 찾기
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty())
    {
        levelData.vPlayerSpawn = vecPlayer[0]->GetPos();
    }

    // 모든 오브젝트 데이터 수집 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue; // 플레이어는 제외

        const vector<CObject*>& vecObj = GetGroupObject((GROUP_TYPE)i);
        for (size_t j = 0; j < vecObj.size(); ++j)
        {
            if (vecObj[j] && !vecObj[j]->IsDead()) // 유효하고 살아있는 오브젝트만
            {
                tLevelObjectData objData;
                objData.eGroupType = (GROUP_TYPE)i;
                objData.vPos = vecObj[j]->GetPos();
                objData.vScale = vecObj[j]->GetScale();
                objData.iSubType = 0; // 추후 확장 가능

                levelData.vecObjects.push_back(objData);
            }
        }
    }

    // 파일로 저장
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strLevelDir = strContentPath + L"level\\";
    wstring strFullPath = strLevelDir + _strFileName + L".lvl";

    // 디렉토리가 없으면 생성
    CreateDirectory(strLevelDir.c_str(), nullptr);

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"wb");

    if (pFile)
    {
        // 버전 정보
        fwrite(&levelData.iVersion, sizeof(int), 1, pFile);

        // 레벨 이름 길이 및 이름
        size_t nameLen = levelData.strLevelName.length();
        fwrite(&nameLen, sizeof(size_t), 1, pFile);
        fwrite(levelData.strLevelName.c_str(), sizeof(wchar_t), nameLen, pFile);

        // 플레이어 스폰 위치
        fwrite(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

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
        wchar_t szMsg[256];
        swprintf_s(szMsg, L"Level Saved: %s (%d objects)", _strFileName.c_str(), (int)objCount);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
    }
    else
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Failed to save level!");
    }
}

void CScene_Tool::LoadLevel(const wstring& _strFileName)
{
    wstring strContentPath = CPathMgr::GetInst()->GetContentPath();
    wstring strFullPath = strContentPath + L"level\\" + _strFileName + L".lvl";

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, strFullPath.c_str(), L"rb");

    if (!pFile)
    {
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Failed to load level!");
        return;
    }

    // 기존 오브젝트들 삭제 (플레이어 제외)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        if (i == (UINT)GROUP_TYPE::PLAYER) continue;

        vector<CObject*>& vecObj = const_cast<vector<CObject*>&>(GetGroupObject((GROUP_TYPE)i));
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
    m_pSelectedObject = nullptr;

    // 파일에서 데이터 읽기
    tLevelData levelData;

    // 버전 확인
    fread(&levelData.iVersion, sizeof(int), 1, pFile);

    if (levelData.iVersion != 1)
    {
        fclose(pFile);
        SetWindowText(CCore::GetInst()->GetMainHwnd(), L"Unsupported level version!");
        return;
    }

    // 레벨 이름
    size_t nameLen;
    fread(&nameLen, sizeof(size_t), 1, pFile);
    wchar_t* szName = new wchar_t[nameLen + 1];
    fread(szName, sizeof(wchar_t), nameLen, pFile);
    szName[nameLen] = L'\0';
    levelData.strLevelName = szName;
    delete[] szName;

    // 플레이어 스폰 위치
    fread(&levelData.vPlayerSpawn, sizeof(Vec2), 1, pFile);

    // 플레이어 위치 설정
    const vector<CObject*>& vecPlayer = GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayer.empty())
    {
        vecPlayer[0]->SetPos(levelData.vPlayerSpawn);
    }

    // 오브젝트 개수
    size_t objCount;
    fread(&objCount, sizeof(size_t), 1, pFile);

    // 각 오브젝트 생성
    for (size_t i = 0; i < objCount; ++i)
    {
        tLevelObjectData objData;
        fread(&objData, sizeof(tLevelObjectData), 1, pFile);

        CObject* pObj = nullptr;

        switch (objData.eGroupType)
        {
        case GROUP_TYPE::MONSTER:
            pObj = new CMonster;
            break;
            // 추후 다른 오브젝트 타입들 추가
        default:
            continue;
        }

        if (pObj)
        {
            pObj->SetPos(objData.vPos);
            pObj->SetScale(objData.vScale);
            AddObject(pObj, objData.eGroupType);
        }
    }

    fclose(pFile);

    // 성공 메시지
    wchar_t szMsg[256];
    swprintf_s(szMsg, L"Level Loaded: %s (%d objects)", levelData.strLevelName.c_str(), (int)objCount);
    SetWindowText(CCore::GetInst()->GetMainHwnd(), szMsg);
}

void CScene_Tool::QuickSave()
{
    SaveLevel(L"quicksave");
}

void CScene_Tool::QuickLoad()
{
    LoadLevel(L"quicksave");
}