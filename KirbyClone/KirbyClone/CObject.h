#pragma once

class CCollider;
class CTexture;
class CAnimator;
class CRigidBody;

class CObject
{
private:
	Vec2	m_vPos;
	Vec2	m_vScale;
	CCollider* m_pCollider;
	CAnimator* m_pAnimator;
	CRigidBody* m_pRigidBody;
	bool m_bAlive;
	CTexture* m_pTex;

public:
	void SetPos(Vec2 _vPos) { m_vPos = _vPos; }
	void SetScale(Vec2 _vScale) { m_vScale = _vScale; }
	void SetTexture(CTexture* _pTex) { m_pTex = _pTex; }
	Vec2 GetPos() { return m_vPos; }
	Vec2 GetScale() { return m_vScale; }
	CTexture* GetTexture() { return m_pTex; }

	bool IsDead() { return !m_bAlive; }
	void SetDead() { m_bAlive = false; }

	// 가상 함수로 선언 - 자식 클래스에서 재정의 가능
	virtual void Update() = 0;  // 순수 가상 함수 - 자식이 반드시 구현
	virtual void Render(HDC _dc);  // 기본 렌더링 제공

	// 충돌 콜백 함수들 - 자식 클래스에서 필요시 재정의
	virtual void OnCollisionEnter(CCollider* _pOther) {}   // 충돌 시작
	virtual void OnCollision(CCollider* _pOther) {}        // 충돌 중
	virtual void OnCollisionExit(CCollider* _pOther) {}    // 충돌 종료

	void CreateCollider();
	CCollider* GetCollider() { return m_pCollider; }

	void CreateAnimator();
	CAnimator* GetAnimator() { return m_pAnimator; }

	void CreateRigidBody();
	CRigidBody* GetRigidBody() { return m_pRigidBody; }

public:
	CObject();
	virtual ~CObject();  // 가상 소멸자
};