#pragma once

class CCollider;
class CTexture;
class CAnimator;
class CRigidBody;

class CObject
{
public:
    CObject();
    CObject(OBJECT_TYPE _eType);
    virtual ~CObject();  // 가상 소멸자

public:
    // === 핵심 생명주기 함수들 ===
    virtual void Update() = 0;              // 순수 가상 함수 - 자식이 반드시 구현
    virtual void Render(HDC _dc);           // 기본 렌더링 제공

public:
    // === 충돌 콜백 함수들 ===
    virtual void OnCollisionEnter(CCollider* _pOther) {}   // 충돌 시작
    virtual void OnCollision(CCollider* _pOther) {}        // 충돌 중
    virtual void OnCollisionExit(CCollider* _pOther) {}    // 충돌 종료

private:
    // === 충돌 콜백 함수들 구현부는 자식에서 필요시 재정의 ===

public:
    // === 컴포넌트 생성 함수들 ===
    void CreateCollider();
    void CreateAnimator();
    void CreateRigidBody();

public:
    // === 위치 및 크기 관리 ===
    void SetPos(Vec2 _vPos) { m_vPos = _vPos; }
    void SetScale(Vec2 _vScale) { m_vScale = _vScale; }
    Vec2 GetPos() const { return m_vPos; }
    Vec2 GetScale() const { return m_vScale; }

public:
    // === 텍스처 관리 ===
    void SetTexture(CTexture* _pTex) { m_pTex = _pTex; }
    CTexture* GetTexture() const { return m_pTex; }

public:
    // === 생명 상태 관리 ===
    bool IsDead() const { return !m_bAlive; }
    void SetDead() { m_bAlive = false; }
    bool IsActive() const { return m_bAlive; }
    void Revive() { m_bAlive = true; }

public:
    // === 오브젝트 타입 관리 ===
    void SetType(OBJECT_TYPE _eType) { m_eObjectType = _eType; }
    OBJECT_TYPE GetType() const { return m_eObjectType; }

private:
    // === 렌더링 내부 함수들 ===
    float GetRenderScale() const;
    void RenderMain(HDC _dc, const Vec2& _vRenderPos, float _fScale);
    void RenderWithAnimator(HDC _dc, float _fScale);
    void RenderWithTexture(HDC _dc, const Vec2& _vRenderPos, float _fScale);
    void RenderDefaultShape(HDC _dc, const Vec2& _vRenderPos, float _fScale);
    void RenderCollider(HDC _dc);

public:
    // === 컴포넌트 접근자들 ===
    CCollider* GetCollider() const { return m_pCollider; }
    CAnimator* GetAnimator() const { return m_pAnimator; }
    CRigidBody* GetRigidBody() const { return m_pRigidBody; }

private:
    // === 기본 속성들 ===
    Vec2 m_vPos;                    // 위치
    Vec2 m_vScale;                  // 크기
    bool m_bAlive;                  // 생존 상태
    OBJECT_TYPE m_eObjectType;      // 오브젝트 타입

    // === 텍스처 ===
    CTexture* m_pTex;               // 텍스처

    // === 컴포넌트들 ===
    CCollider* m_pCollider;         // 충돌체 컴포넌트
    CAnimator* m_pAnimator;         // 애니메이터 컴포넌트
    CRigidBody* m_pRigidBody;       // 리지드바디 컴포넌트
};