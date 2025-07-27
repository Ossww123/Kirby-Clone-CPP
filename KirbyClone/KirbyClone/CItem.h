#pragma once
#include "CObject.h"

class CItem : public CObject
{
private:
    OBJECT_TYPE m_eItemType;
    int m_iValue;       // 점수나 효과값
    bool m_bCollected;  // 수집 여부

public:
    virtual void Update() override
    {
        // 임시로 기본 업데이트만 구현
        // TODO: 아이템별 특수 동작 구현
    }

    void SetItemType(OBJECT_TYPE _eType) { m_eItemType = _eType; }
    void SetValue(int _iValue) { m_iValue = _iValue; }
    OBJECT_TYPE GetItemType() { return m_eItemType; }
    int GetValue() { return m_iValue; }

    virtual void OnCollisionEnter(CCollider* _pOther) override
    {
        // TODO: 플레이어와 충돌 시 수집 처리
    }

public:
    CItem()
        : m_eItemType(OBJECT_TYPE::ITEM_STAR)
        , m_iValue(100)
        , m_bCollected(false)
    {
        // 기본 아이템 설정
    }
    ~CItem() {}
};