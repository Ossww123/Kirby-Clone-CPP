#pragma once

class CObject
{
private:
	Vec2	m_vPos;
	Vec2	m_vScale;

public:
	void SetPos(Vec2 _vPos) { m_vPos = _vPos; }
	void SetScale(Vec2 _vScale) { m_vScale = _vScale; }

	Vec2 GetPos() { return m_vPos; }
	Vec2 GetScale() { return m_vScale; }

	// 가상 함수로 선언 - 자식 클래스에서 재정의 가능
	virtual void Update() = 0;  // 순수 가상 함수 - 자식이 반드시 구현
	virtual void Render(HDC _dc);  // 기본 렌더링 제공

public:
	CObject();
	virtual ~CObject();  // 가상 소멸자
};