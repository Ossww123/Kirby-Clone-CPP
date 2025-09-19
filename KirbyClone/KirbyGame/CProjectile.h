#pragma once
#include "CObject.h"

class CProjectile : public CObject
{
public:
    CProjectile();
    CProjectile(PROJECTILE_TYPE _eType);
    virtual ~CProjectile();

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

public:
    virtual void OnCollisionEnter(CCollider* _pOther) override;

public:
    // === 투사체 속성 설정 ===
    void SetProjectileType(PROJECTILE_TYPE _eType) { m_eProjectileType = _eType; }
    PROJECTILE_TYPE GetProjectileType() const { return m_eProjectileType; }
    
    void SetDirection(Vec2 _vDir) { m_vDirection = _vDir; }
    Vec2 GetDirection() const { return m_vDirection; }
    
    void SetSpeed(float _fSpeed) { m_fSpeed = _fSpeed; }
    float GetSpeed() const { return m_fSpeed; }
    
    void SetDamage(float _fDamage) { m_fDamage = _fDamage; }
    float GetDamage() const { return m_fDamage; }
    
    void SetLifeTime(float _fLifeTime) { m_fMaxLifeTime = _fLifeTime; }
    float GetLifeTime() const { return m_fMaxLifeTime; }

    void SetOwnerType(GROUP_TYPE _eOwner) { m_eOwnerType = _eOwner; }
    GROUP_TYPE GetOwnerType() const { return m_eOwnerType; }
    
    // === 회전 빔용 특수 설정 ===
    void SetRotationData(Vec2 _vCenter, float _fRadius, float _fStartAngle, float _fEndAngle, float _fDuration);

private:
    // === 투사체 이동 처리 ===
    void UpdateMovement();
    void UpdateLifeTime();
    void CheckBounds();

private:
    // === 투사체 타입별 초기화 ===
    void InitializeByType();
    void CreateAnimation();
    void SetAnimationByType();

private:
    // === 투사체 속성 ===
    PROJECTILE_TYPE m_eProjectileType;  // 투사체 종류
    Vec2 m_vDirection;                  // 이동 방향 (정규화된 벡터)
    float m_fSpeed;                     // 현재 이동 속도
    float m_fInitialSpeed;              // 초기 속도 (감속 계산용)
    float m_fDeceleration;              // 감속도 (속도 감소율)
    float m_fDamage;                    // 데미지
    
    // === 생명주기 관리 ===
    float m_fAccTime;                   // 누적 시간
    float m_fMaxLifeTime;               // 최대 생존 시간
    
    // === 소유자 정보 ===
    GROUP_TYPE m_eOwnerType;            // 발사한 주체 (PLAYER, MONSTER 등)
    
    // === 회전 빔용 특수 데이터 ===
    bool m_bIsRotatingBeam;             // 회전 빔 여부
    Vec2 m_vRotationCenter;             // 회전 중심점
    float m_fRotationRadius;            // 회전 반지름
    float m_fStartAngle;                // 시작 각도
    float m_fEndAngle;                  // 끝 각도
    float m_fRotationDuration;          // 회전 지속시간
    float m_fRotationTimer;             // 회전 경과시간
    
private:
    // === 회전 빔 전용 업데이트 ===
    void UpdateRotatingBeam();
};