#pragma once


class CKeyMgr
{
    SINGLE(CKeyMgr);
private:
    vector<tKeyInfo> m_vecKey;
    Vec2 m_vMousePos;

public:
    void init();
    void update();

    KEY_STATE GetKeyState(KEY _eKey) { return m_vecKey[(int)_eKey].eState; }
    bool IsKeyTap(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::TAP; }
    bool IsKeyHold(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::HOLD; }
    bool IsKeyAway(KEY _eKey) { return m_vecKey[(int)_eKey].eState == KEY_STATE::AWAY; }

    // 마우스 관련 함수
    Vec2 GetMousePos() { return m_vMousePos; }      // 스크린 좌표
    Vec2 GetMouseWorldPos();                        // 월드 좌표 (Camera 변환 적용)

private:
    void UpdateMousePos();  // 마우스 좌표 업데이트
};

