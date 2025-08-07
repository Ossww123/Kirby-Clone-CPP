#pragma once
#include "CSpecialObject.h"

// CSpecialObject를 상속받는 문 클래스
class CDoor : public CSpecialObject
{
private:
    SCENE_TYPE      m_eTargetScene;         // 이동할 씬
    Vec2            m_vTargetPosition;      // 목표 씬에서의 플레이어 위치
    wstring         m_strDoorID;            // 문 고유 식별자
    wstring         m_strTargetDoorID;      // 연결된 문의 ID

    bool            m_bPlayerNear;          // 플레이어가 근처에 있는지
    bool            m_bCanInteract;         // 상호작용 가능한지

    float           m_fInteractionRange;    // 상호작용 가능 거리
    float           m_fAnimTimer;           // 애니메이션 타이머

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    virtual void OnCollisionEnter(CCollider* _pOther) override;
    virtual void OnCollisionExit(CCollider* _pOther) override;

private:
    void CheckPlayerInteraction();      // 플레이어 상호작용 체크
    void ProcessDoorTransition();       // 문 이동 처리
    void RenderInteractionUI(HDC _dc);  // 상호작용 UI 렌더링
    void RenderDoorVisual(HDC _dc);     // 문 시각적 표현

public:
    // Setter 함수들
    void SetTargetScene(SCENE_TYPE _eScene) { m_eTargetScene = _eScene; }
    void SetTargetPosition(Vec2 _vPos) { m_vTargetPosition = _vPos; }
    void SetDoorID(const wstring& _strID) { m_strDoorID = _strID; }
    void SetTargetDoorID(const wstring& _strID) { m_strTargetDoorID = _strID; }
    void SetInteractionRange(float _fRange) { m_fInteractionRange = _fRange; }

    // Getter 함수들
    SCENE_TYPE GetTargetScene() const { return m_eTargetScene; }
    Vec2 GetTargetPosition() const { return m_vTargetPosition; }
    const wstring& GetDoorID() const { return m_strDoorID; }
    const wstring& GetTargetDoorID() const { return m_strTargetDoorID; }
    bool CanInteract() const { return m_bCanInteract; }
    float GetInteractionRange() const { return m_fInteractionRange; }

public:
    CDoor();
    virtual ~CDoor();
};