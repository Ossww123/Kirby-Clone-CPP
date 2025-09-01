#pragma once
#include "CSpecialObject.h"

class CAnimator;

// CSpecialObject를 상속받는 문 클래스
class CDoor : public CSpecialObject
{
public:
    CDoor();
    virtual ~CDoor();

public:
    // === 메인 진입점 함수들 ===
    void Update() override;
    void Render(HDC _dc) override;

public:
    // === 충돌 처리 ===
    void OnCollisionEnter(CCollider* _pOther) override;
    void OnCollisionExit(CCollider* _pOther) override;

private:
    // === 상호작용 시스템 ===
    void CheckPlayerInteraction();

    // === 씬 전환 처리 ===
    void ProcessDoorTransition();

private:
    // === 렌더링 시스템 ===
    void RenderDoorVisual(HDC _dc);
    void RenderInteractionUI(HDC _dc);

public:
    // === Setter 함수들 ===
    void SetTargetScene(SCENE_TYPE _eScene) { m_eTargetScene = _eScene; }
    void SetTargetPosition(Vec2 _vPos) { m_vTargetPosition = _vPos; }

    // === Getter 함수들 ===
    SCENE_TYPE GetTargetScene() const { return m_eTargetScene; }
    Vec2 GetTargetPosition() const { return m_vTargetPosition; }
    bool CanInteract() const { return m_bCanInteract; }

private:
    // === 씬 이동 정보 ===
    SCENE_TYPE      m_eTargetScene;         // 이동할 씬
    Vec2            m_vTargetPosition;      // 목표 씬에서의 플레이어 위치

    // === 상호작용 상태 ===
    bool            m_bPlayerNear;          // 플레이어 근처에 있는지
    bool            m_bCanInteract;         // 상호작용 가능한지
};