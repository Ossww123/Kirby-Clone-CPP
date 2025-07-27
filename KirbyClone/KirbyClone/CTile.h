#pragma once
#include "CObject.h"

class CTile : public CObject
{
private:
    OBJECT_TYPE m_eTileType;
    bool m_bSolid;          // 충돌 여부
    bool m_bHarmful;        // 데미지 여부 (가시 등)

public:
    virtual void Update() override
    {
        // 타일은 기본적으로 움직이지 않음
        // TODO: 특수 타일 (움직이는 플랫폼 등) 구현
    }

    void SetTileType(OBJECT_TYPE _eType) { m_eTileType = _eType; }
    void SetSolid(bool _bSolid) { m_bSolid = _bSolid; }
    void SetHarmful(bool _bHarmful) { m_bHarmful = _bHarmful; }

    OBJECT_TYPE GetTileType() { return m_eTileType; }
    bool IsSolid() { return m_bSolid; }
    bool IsHarmful() { return m_bHarmful; }

    virtual void OnCollisionEnter(CCollider* _pOther) override
    {
        // TODO: 플레이어와 충돌 시 효과 처리 (데미지, 워프 등)
    }

public:
    CTile()
        : m_eTileType(OBJECT_TYPE::TILE_GROUND)
        , m_bSolid(true)
        , m_bHarmful(false)
    {
        // 기본 타일 설정
    }
    ~CTile() {}
};