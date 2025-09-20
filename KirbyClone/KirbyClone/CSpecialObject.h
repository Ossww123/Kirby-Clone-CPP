#pragma once
#include "CObject.h"

class CSpecialObject : public CObject
{
public:
    CSpecialObject();
    virtual ~CSpecialObject();

public:
    // === 핵심 생명주기 함수들 ===
    void Update() override;               // 자식 클래스에서 필요시 구현
    void Render(HDC _dc) override;          // CObject의 기본 렌더링 사용

public:
    // === 상호작용 시스템 ===
    virtual void OnInteract(CObject* _pActor) {}
    virtual bool CanInteract(CObject* _pActor) const { return m_bIsActive && m_bIsInteractable; }

public:
    // === Setter 함수들 ===
    void SetSpecialType(OBJECT_TYPE _eType) { m_eSpecialType = _eType; }
    void SetActive(bool _bActive) { m_bIsActive = _bActive; }
    void SetInteractable(bool _bInteractable) { m_bIsInteractable = _bInteractable; }
    void SetInteractionRange(float _fRange) { m_fInteractionRange = _fRange; }

    // === Getter 함수들 ===
    OBJECT_TYPE GetSpecialType() const { return m_eSpecialType; }
    bool IsActive() const { return m_bIsActive; }
    bool IsInteractable() const { return m_bIsInteractable; }
    float GetInteractionRange() const { return m_fInteractionRange; }

protected:
    // === 특수 오브젝트 속성 ===
    OBJECT_TYPE     m_eSpecialType;         // 특수 오브젝트 타입
    bool            m_bIsActive;            // 활성화 상태
    bool            m_bIsInteractable;      // 상호작용 가능 여부
    float           m_fInteractionRange;    // 상호작용 가능 범위
};