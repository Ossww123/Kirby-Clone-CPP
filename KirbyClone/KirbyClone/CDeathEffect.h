#pragma once
#include "CObject.h"

// 몬스터 죽음 이펙트 클래스
class CDeathEffect : public CObject
{
public:
    CDeathEffect();
    virtual ~CDeathEffect();

public:
    // === 생명주기 함수 ===
    void Init();
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // === 이펙트 설정 ===
    void SetLifeTime(float _fLifeTime) { m_fLifeTime = _fLifeTime; }
    
    // === 상태 확인 ===
    bool IsAlive() const { return m_fTimer < m_fLifeTime; }

private:
    // === 시각 효과 ===
    float       m_fTimer;           // 생존 시간 타이머
    float       m_fLifeTime;        // 최대 생존 시간
    
    // === 애니메이션 ===
    bool        m_bAnimationLoaded; // 애니메이션 로드 여부

private:
    // === 내부 함수 ===
    void LoadAnimation();           // 애니메이션 로드
};