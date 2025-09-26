#pragma once
#include "CObject.h"

// 화면 고정 UI의 기본 베이스 (카메라 이동에 영향받지 않음)
class CUIElement : public CObject
{
public:
    CUIElement();
    ~CUIElement() override = default;

    // 화면 고정 UI는 월드 물리 불필요 → 기본 Update는 비움
    void Update() override {}

    // Render는 화면 좌표로 직접 그리게끔 오버라이드
    void Render(HDC dc) override;

    // 표시/비표시
    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const { return m_visible; }

protected:
    // 파생 클래스가 화면좌표를 받아 실제 그리는 함수
    virtual void RenderUI(HDC dc, const Vec2& screenPos) = 0;

private:
    bool m_visible{ true };
};
