#pragma once
#include "CObject.h"

class CTile;

class CAbilityStar : public CObject
{
public:
    CAbilityStar();
    CAbilityStar(COPY_ABILITY _eAbility);
    virtual ~CAbilityStar();

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

public:
    // === 충돌 콜백 함수 ===
    virtual void OnCollisionEnter(CCollider* _pOther) override;
    virtual void OnCollision(CCollider* _pOther) override;

public:
    // === 능력별 속성 ===
    void SetAbility(COPY_ABILITY _eAbility) { m_eAbility = _eAbility; }
    COPY_ABILITY GetAbility() const { return m_eAbility; }
    
    void SetInitialVelocity(Vec2 _vVelocity) { 
        m_vInitialVelocity = _vVelocity; 
        m_vVelocity = _vVelocity; 
    }
    bool IsOnGround() const { return m_bOnGround; }

private:
    // === 물리 시뮬레이션 ===
    void UpdatePhysics();
    void CheckGroundCollision();
    void ApplyBounce();
    
    // === 타일 충돌 처리 ===
    void HandleTileCollision(CObject* _pTile);

private:
    // === 능력별 속성 ===
    COPY_ABILITY m_eAbility;        // 보유한 능력
    
    // === 생명주기 관리 ===
    float m_fLifeTime;              // 현재 생존 시간
    float m_fMaxLifeTime;           // 최대 생존 시간 (10초)
    
    // === 물리 시뮬레이션 ===
    Vec2 m_vVelocity;               // 현재 속도 (x, y)
    Vec2 m_vInitialVelocity;        // 초기 속도 (사인파 궤도용)
    float m_fGravity;               // 중력 가속도
    float m_fBounceFactorX;         // X축 반발 계수
    float m_fBounceFactorY;         // Y축 반발 계수
    float m_fFriction;              // 마찰 계수
    bool m_bOnGround;               // 땅에 닿았는지
    
    // === 시각적 효과 ===
    float m_fBlinkTimer;            // 깜빡임 타이머 (사라지기 전)
    bool m_bVisible;                // 깜빡임 상태
};