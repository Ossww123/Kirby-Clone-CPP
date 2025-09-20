#include "gamePCH.h"
#include "CKeyMgr.h"
#include "CCore.h"
#include "CCamera.h"

int g_arrVK[(int)KEY::LAST] =
{
    VK_LEFT,
    VK_RIGHT,
    VK_UP,
    VK_DOWN,

    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',

    // 숫자 키 추가
    '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0',

    VK_MENU,    // ALT 키 (VK_MENU가 Alt 키의 가상 키 코드)
    VK_HOME,    // HOME 키
    VK_BACK,

    VK_SPACE,
    VK_RETURN,
    VK_ESCAPE,
    VK_TAB,
    VK_SHIFT,
    VK_CONTROL,

    VK_LBUTTON,
    VK_RBUTTON,
    VK_MBUTTON,

};

CKeyMgr::CKeyMgr()
    : m_vMousePos{}
{}

CKeyMgr::~CKeyMgr()
{}

void CKeyMgr::init()
{
    m_vecKey.resize((int)KEY::LAST);
}

void CKeyMgr::update()
{
    // 윈도우가 포커스 상태인지 확인
    HWND hWnd = CCore::GetInst()->GetMainHwnd();
    HWND hFocusedWnd = GetFocus();

    // 윈도우가 포커스 상태가 아니라면 모든 키를 AWAY상태로 처리
    if (nullptr == hFocusedWnd || hWnd != hFocusedWnd)
    {
        for (size_t i = 0; i < (size_t)KEY::LAST; ++i)
        {
            m_vecKey[i].bPrevPush = false;

            if (KEY_STATE::TAP == m_vecKey[i].eState || KEY_STATE::HOLD == m_vecKey[i].eState)
            {
                m_vecKey[i].eState = KEY_STATE::AWAY;
            }
            else if (KEY_STATE::AWAY == m_vecKey[i].eState)
            {
                m_vecKey[i].eState = KEY_STATE::NONE;
            }
        }
        return;
    }

    // 마우스 좌표 업데이트
    UpdateMousePos();

    // 모든 키에 대해 상태 업데이트
    for (size_t i = 0; i < (size_t)KEY::LAST; ++i)
    {
        // 키가 현재 눌려있는지 확인
        if (GetAsyncKeyState(g_arrVK[i]) & 0x8000)
        {
            if (m_vecKey[i].bPrevPush)
            {
                // 이전에도 눌려있었다면 HOLD
                m_vecKey[i].eState = KEY_STATE::HOLD;
            }
            else
            {
                // 이전에 안눌려있었다면 TAP
                m_vecKey[i].eState = KEY_STATE::TAP;
            }
            m_vecKey[i].bPrevPush = true;
        }
        else
        {
            if (m_vecKey[i].bPrevPush)
            {
                // 이전에 눌려있었다면 AWAY
                m_vecKey[i].eState = KEY_STATE::AWAY;
            }
            else
            {
                // 이전에도 안눌려있었다면 NONE
                m_vecKey[i].eState = KEY_STATE::NONE;
            }
            m_vecKey[i].bPrevPush = false;
        }
    }
}

void CKeyMgr::UpdateMousePos()
{
    // 마우스 스크린 좌표 얻기
    POINT ptMouse;
    GetCursorPos(&ptMouse);
    ScreenToClient(CCore::GetInst()->GetMainHwnd(), &ptMouse);

    m_vMousePos.x = (float)ptMouse.x;
    m_vMousePos.y = (float)ptMouse.y;
}

Vec2 CKeyMgr::GetMouseWorldPos()
{
    // 스크린 좌표를 월드 좌표로 변환
    return CCamera::GetInst()->GetRealPos(m_vMousePos);
}