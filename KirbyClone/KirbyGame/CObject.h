#pragma once

class CCollider;
class CAnimator;
class CRigidBody;

struct Transform2D {
    Vec2  position{ 0, 0 };
    Vec2  scale{ 1, 1 };     // 픽셀아트면 1,1 고정도 OK
    bool  flipX{ false };    // 좌우 반전은 보통 scale.x 음수 대신 bool로 권장(픽셀 깨짐 방지)
    float rotationRad{ 0.f }; // 기본 0, 확장용
};

class CObject
{
public:
    CObject();
    CObject(OBJECT_TYPE _eType);
    virtual ~CObject();  // 가상 소멸자

    // 복사/이동 금지 (소유 포인터 보호)
    CObject(const CObject&) = delete;
    CObject& operator=(const CObject&) = delete;
    CObject(CObject&&) = delete;
    CObject& operator=(CObject&&) = delete;

public:
    // === 핵심 생명주기 ===
    virtual void Update() = 0;
    virtual void Render(HDC _dc);
    virtual void OnDestroy() {}

protected:
    // 렌더 훅 — 파생에서 전/후 오버레이 쉽게 추가
    virtual void OnPreRender(HDC) {}
    virtual void OnPostRender(HDC) {}

public:
    // === 충돌 콜백 ===
    virtual void OnCollisionEnter(CCollider* _pOther) {}
    virtual void OnCollision(CCollider* _pOther) {}
    virtual void OnCollisionExit(CCollider* _pOther) {}

public:
    // === 컴포넌트 생성 ===
    void CreateCollider();
    void CreateAnimator();
    void CreateRigidBody();

public:
    // === Transform 접근 ===
    void SetPos(Vec2 _vPos) { m_Transform.position = _vPos; }
    void SetScale(Vec2 _vScale) { m_Transform.scale = _vScale; }
    Vec2 GetPos() const { return m_Transform.position; }
    Vec2 GetScale() const { return m_Transform.scale; }

    // flip/rotation
    bool  IsFlipX() const { return m_Transform.flipX; }
    void  SetFlipX(bool v) { m_Transform.flipX = v; }
    float GetRotationRad() const { return m_Transform.rotationRad; }
    void  SetRotationRad(float r) { m_Transform.rotationRad = r; }

    // 전체 Transform 핸들
    Transform2D& Transform() { return m_Transform; }
    const Transform2D& Transform() const { return m_Transform; }


public:
    // === 생명 상태 ===
    bool IsAlive() const { return m_bAlive; }
    void SetDead() { m_bAlive = false; }
    void SetAlive() { m_bAlive = true; }

public:
    // === 오브젝트 타입 ===
    void        SetType(OBJECT_TYPE t) { m_ObjectType = t; }
    OBJECT_TYPE GetType() const { return m_ObjectType; }

    void        SetGroup(GROUP_TYPE g) { m_Group = g; }
    GROUP_TYPE  GetGroup() const { return m_Group; }

    bool IsInGroup(GROUP_TYPE g) const { return m_Group == g; }

private:
    // === 내부 렌더 ===
    void RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale);
    void RenderWithAnimator(HDC _dc, float _fScale);

    // 디버그 전용 (빌드/매크로에 따라 제외)
    void RenderDefaultShape(HDC _dc, const Vec2& _vRenderPos, float _fScale);
    void RenderCollider(HDC _dc);

public:
    // === 컴포넌트 접근자 === (const 오버로드 포함)
    CCollider* GetCollider() { return m_pCollider.get(); }
    const CCollider* GetCollider() const { return m_pCollider.get(); }

    CAnimator* GetAnimator() { return m_pAnimator.get(); }
    const CAnimator* GetAnimator() const { return m_pAnimator.get(); }

    CRigidBody* GetRigidBody() { return m_pRigidBody.get(); }
    const CRigidBody* GetRigidBody() const { return m_pRigidBody.get(); }

private:
    // === 기본 속성 ===
    Transform2D m_Transform{};
    bool        m_bAlive{ true };

    OBJECT_TYPE m_ObjectType{ OBJECT_TYPE::END }; // 세부 식별자
    GROUP_TYPE  m_Group{ GROUP_TYPE::DEFAULT };   // 충돌/필터용 그룹

    // === 컴포넌트 (unique_ptr) ===
    std::unique_ptr<CCollider>  m_pCollider;
    std::unique_ptr<CAnimator>  m_pAnimator;
    std::unique_ptr<CRigidBody> m_pRigidBody;
};