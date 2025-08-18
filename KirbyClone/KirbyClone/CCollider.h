#pragma once

class CObject;

class CCollider
{
public:
    // === 생성자 & 소멸자 ===
    CCollider();
    ~CCollider();

public:
    // === 핵심 생명주기 함수들 ===
    void FinalUpdate();
    void Render(HDC _dc);
    void RenderScaled(HDC _dc, float _fScale);

public:
    // === 충돌 검사 ===
    bool IsCollision(CCollider* _pOther);

public:
    // === 충돌 이벤트 처리 ===
    void OnCollisionEnter(CCollider* _pOther);  // 충돌 시작
    void OnCollision(CCollider* _pOther);       // 충돌 중
    void OnCollisionExit(CCollider* _pOther);   // 충돌 끝

public:
    // === 위치 및 크기 관리 ===
    void SetOffsetPos(Vec2 _vPos) { m_vOffsetPos = _vPos; }
    void SetScale(Vec2 _vScale) { m_vScale = _vScale; }
    Vec2 GetOffsetPos() const { return m_vOffsetPos; }
    Vec2 GetScale() const { return m_vScale; }
    Vec2 GetFinalPos();         // 최종 위치 (오브젝트 위치 + 오프셋)

public:
    // === 소유자 및 ID 관리 ===
    CObject* GetOwner() const { return m_pOwner; }
    UINT GetID() const { return m_iID; }

public:
    // === 충돌 목록 관리 ===
    const vector<CCollider*>& GetCollidingColliders() const { return m_vecCollidingColliders; }
    void AddCollidingCollider(CCollider* _pOther);
    void RemoveCollidingCollider(CCollider* _pOther);
    void ClearCollidingColliders() { m_vecCollidingColliders.clear(); }
    bool IsCollidingWith(CCollider* _pOther) const;

private:
    // === 정적 변수 ===
    static UINT g_iNextID;      // 다음 ID 생성용

private:
    // === 멤버 변수들 ===
    CObject*    m_pOwner;          // 이 충돌체를 소유한 오브젝트
    Vec2        m_vOffsetPos;   // 오브젝트 중심으로부터의 오프셋
    Vec2        m_vScale;       // 충돌체 크기
    UINT        m_iID;          // 충돌체 고유 ID

    // 현재 충돌 중인 콜라이더들의 목록
    vector<CCollider*> m_vecCollidingColliders;

    friend class CObject;
};