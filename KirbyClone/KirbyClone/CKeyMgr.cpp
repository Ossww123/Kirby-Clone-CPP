#include "pch.h"
#include "CKeyMgr.h"
#include "CCore.h"

int g_arrVK[(int)KEY::LAST] =
{
    VK_LEFT,
    VK_RIGHT,
    VK_UP,
    VK_DOWN,

    'Q', 'W', 'E', 'R', 'T', 'Y',
    'A', 'S', 'D', 'F', 'G', 'H',
    'Z', 'X', 'C', 'V', 'B',

    VK_SPACE,
    VK_RETURN,
    VK_ESCAPE,
};

CKeyMgr::CKeyMgr()
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
        for (int i = 0; i < (int)KEY::LAST; ++i)
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

    // 모든 키에 대해 상태 업데이트
    for (int i = 0; i < (int)KEY::LAST; ++i)
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
