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

    // 현재 충돌 중인 콜라이더들의 목록
    vector<CCollider*> m_vecCollidingColliders;

public:
    void SetOffsetPos(Vec2 _vPos) { m_vOffsetPos = _vPos; }
    void SetScale(Vec2 _vScale) { m_vScale = _vScale; }

    Vec2 GetOffsetPos() { return m_vOffsetPos; }
    Vec2 GetScale() { return m_vScale; }
    Vec2 GetFinalPos();         // 최종 위치 (오브젝트 위치 + 오프셋)

    CObject* GetOwner() { return m_pOwner; }
    UINT GetID() { return m_iID; }

    // 충돌 목록 관리 함수들
    const vector<CCollider*>& GetCollidingColliders() const { return m_vecCollidingColliders; }
    void AddCollidingCollider(CCollider* _pOther);
    void RemoveCollidingCollider(CCollider* _pOther);
    void ClearCollidingColliders() { m_vecCollidingColliders.clear(); }
    bool IsCollidingWith(CCollider* _pOther) const;

public:
    void FinalUpdate();
    void Render(HDC _dc);

    // 두 충돌체가 충돌했는지 검사
    bool IsCollision(CCollider* _pOther);

    // 충돌 콜백 함수
    void OnCollisionEnter(CCollider* _pOther);  // 충돌 시작
    void OnCollision(CCollider* _pOther);       // 충돌 중
    void OnCollisionExit(CCollider* _pOther);   // 충돌 끝

public:
    CCollider();
    ~CCollider();

    friend class CObject;
};