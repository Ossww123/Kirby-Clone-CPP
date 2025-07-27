#pragma once
#include "CObject.h"

class CCollider;

class CTile : public CObject
{
private:
    OBJECT_TYPE m_eTileType;
    bool m_bSolid;          // 충돌 여부
    bool m_bHarmful;        // 데미지 여부 (가시 등)

public:
    virtual void Update();
    virtual void Render(HDC _dc);

    void SetTileType(OBJECT_TYPE _eType) { m_eTileType = _eType; }
    void SetSolid(bool _bSolid) { m_bSolid = _bSolid; }
    void SetHarmful(bool _bHarmful) { m_bHarmful = _bHarmful; }

    OBJECT_TYPE GetTileType() { return m_eTileType; }
    bool IsSolid() { return m_bSolid; }
    bool IsHarmful() { return m_bHarmful; }

    virtual void OnCollisionEnter(CCollider* _pOther);

public:
    CTile();
    ~CTile();
};