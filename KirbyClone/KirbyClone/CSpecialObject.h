#pragma once
#include "CObject.h"

class CSpecialObject : public CObject
{
protected:
    OBJECT_TYPE     m_eSpecialType;         // 특수 오브젝트 타입
    bool            m_bIsActive;            // 활성화 상태
    bool            m_bIsInteractable;      // 상호작용 가능 여부
    float           m_fInteractionRange;    // 상호작용 가능 범위

    // 공통 애니메이션
    float           m_fAnimTimer;           // 애니메이션 타이머
    int             m_iAnimFrame;           // 현재 애니메이션 프레임

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // 상호작용 처리 (자식 클래스에서 구현)
    virtual void OnInteract(CObject* _pActor) {}
    virtual bool CanInteract(CObject* _pActor) { return m_bIsActive && m_bIsInteractable; }

    // Setter
    void SetSpecialType(OBJECT_TYPE _eType) { m_eSpecialType = _eType; }
    void SetActive(bool _bActive) { m_bIsActive = _bActive; }
    void SetInteractable(bool _bInteractable) { m_bIsInteractable = _bInteractable; }
    void SetInteractionRange(float _fRange) { m_fInteractionRange = _fRange; }

    // Getter
    OBJECT_TYPE GetSpecialType() const { return m_eSpecialType; }
    bool IsActive() const { return m_bIsActive; }
    bool IsInteractable() const { return m_bIsInteractable; }
    float GetInteractionRange() const { return m_fInteractionRange; }

public:
    CSpecialObject();
    virtual ~CSpecialObject();
};