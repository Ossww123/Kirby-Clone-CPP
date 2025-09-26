#include "gamePCH.h"
#include "CUIElement.h"

// 기본 생성: 그룹을 UI로 (없으면 DEFAULT로 두셔도 됩니다)
CUIElement::CUIElement() {
    SetGroup(GROUP_TYPE::UI);     // 없으면 주석 처리
    // UI는 물리/충돌이 필요 없다면 컴포넌트 생성하지 않음
    // CreateCollider() 등을 호출하지 않는다.
}

// 화면 고정: 카메라 변환을 쓰지 않고, Transform().position을 곧바로 화면 좌표로 사용
void CUIElement::Render(HDC dc)
{
    if (!IsVisible() || !IsAlive())
        return;

    const Vec2 screenPos = GetPos(); // 화면 좌표로 사용
    RenderUI(dc, screenPos);
}
