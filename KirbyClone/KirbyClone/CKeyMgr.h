#pragma once


class CKeyMgr
{
    SINGLE(CKeyMgr);
private:
    vector<tKeyInfo> m_vecKey;

public:
    void init();
    void update();

    KEY_STATE GetKeyState(KEY _eKey) { return m_vecKey[(int)_eKey].eState; }
    bool IsKeyTap(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::TAP; }
    bool IsKeyHold(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::HOLD; }
    bool IsKeyAway(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::AWAY; }
};

