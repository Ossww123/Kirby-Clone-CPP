#pragma once

class CObject;

class CCollider
{
private:
    CObject*    m_pOwner;       // 이 충돌체를 소유한 오브젝트
    Vec2        m_vOffsetPos;   // 오브젝트 중심으로부터의 오프셋
    Vec2        m_vScale;       // 충돌체 크기

    UINT        m_iID;          // 충돌체 고유 ID
    static UINT g_iNextID;      // 다음 ID 생성용

public:
    void SetOffsetPos(Vec2 _vPos) { m_vOffsetPos = _vPos; }
    void SetScale(Vec2 _vScale) { m_vScale = _vScale; }

    Vec2 GetOffsetPos() { return m_vOffsetPos; }
    Vec2 GetScale() { return m_vScale; }
    Vec2 GetFinalPos();         // 최종 위치 (오브젝트 위치 + 오프셋)

    CObject* GetOwner() { return m_pOwner; }
    UINT GetID() { return m_iID; }

public:
    void FinalUpdate();
    void Render(HDC _dc);

    // 두 충돌체가 충돌했는지 검사
    bool IsCollision(CCollider* _pOther);

public:
    CCollider();
    ~CCollider();

    friend class CObject;
};