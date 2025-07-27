#pragma once
#include "CObject.h"

class CSpecialObject : public CObject
{
private:
    OBJECT_TYPE m_eSpecialType;
    bool m_bActive;         // 활성화 여부
    float m_fTimer;         // 타이머 (필요시)

public:
    virtual void Update() override
    {
        // TODO: 특수 오브젝트별 동작 구현
        // 문 열기/닫기, 스위치 토글, 거울 반사 등
    }

    void SetSpecialType(OBJECT_TYPE _eType) { m_eSpecialType = _eType; }
    void SetActive(bool _bActive) { m_bActive = _bActive; }

    OBJECT_TYPE GetSpecialType() { return m_eSpecialType; }
    bool IsActive() { return m_bActive; }

    virtual void OnCollisionEnter(CCollider* _pOther) override
    {
        // TODO: 플레이어와 상호작용 처리
    }

public:
    CSpecialObject()
        : m_eSpecialType(OBJECT_TYPE::OBJECT_DOOR)
        , m_bActive(true)
        , m_fTimer(0.f)
    {
        // 기본 특수 오브젝트 설정
    }
    ~CSpecialObject() {}
};