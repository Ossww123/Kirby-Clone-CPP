#pragma once
#include "CBasicMonster.h"

class CApple : public CBasicMonster
{
public:
    CApple();
    virtual ~CApple();

public:
    // === 가상 함수 구현 ===
    void Move() override;                           // 중력 적용 낙하

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

public:
    // === 사과 전용 함수들 ===
    void SetGravity(bool _bGravity) { m_bGravityEnabled = _bGravity; }
    void SetFallSpeed(float _fSpeed) { m_fFallSpeed = _fSpeed; }
    void SetLifetime(float _fLifetime) { m_fLifetime = _fLifetime; }
    void StartWarningPhase(float _fDuration = 1.0f); // 전조 단계 시작

    // === 충돌 처리 ===
    void OnCollisionEnter(CCollider* _pOther) override;
    void OnCollision(CCollider* _pOther) override;

private:
    // === 사과 전용 업데이트 ===
    void UpdateWarning();                           // 전조 단계 업데이트
    void UpdateFalling();                           // 낙하 처리
    void UpdateBouncing();                          // 바운스 처리
    void UpdateRolling();                           // 굴러가기 처리
    void CheckGroundCollision();                    // 바닥 충돌 체크
    void HandlePlayerHit();                         // 플레이어 충돌 처리
    void StartRolling();                            // 굴러가기 시작

private:
    // === 사과 상태 관리 ===
    enum class APPLE_STATE
    {
        WARNING,        // 전조 단계 (떨어지기 전 예고)
        FALLING,        // 낙하 중
        BOUNCING,       // 바운스 중
        ROLLING         // 굴러가는 중
    };
    APPLE_STATE m_eAppleState;                      // 현재 상태
    
    // === 사과 속성들 ===
    bool    m_bGravityEnabled;                      // 중력 적용 여부
    float   m_fFallSpeed;                           // 낙하 속도
    float   m_fGravityAccel;                        // 중력 가속도
    float   m_fMaxFallSpeed;                        // 최대 낙하 속도
    
    // === 전조 단계 관리 ===
    float   m_fWarningDuration;                     // 전조 지속 시간
    float   m_fWarningTimer;                        // 전조 타이머
    
    // === 굴러가기 관리 ===
    float   m_fRollSpeed;                           // 굴러가기 속도
    int     m_iRollDirection;                       // 굴러가는 방향 (-1: 왼쪽, 1: 오른쪽)
    bool    m_bRollingStarted;                      // 굴러가기 시작 여부
    
    // === 생명주기 관리 ===
    float   m_fLifetime;                            // 생존 시간
    float   m_fLifetimeTimer;                       // 생존 타이머
    
    // === 바운스 관리 ===
    float   m_fBounceSpeed;                         // 바운스 속도
    float   m_fBounceHorizontalSpeed;               // 바운스 시 수평 속도
    bool    m_bHasBounced;                          // 이미 바운스 했는지 여부
    
    // === 충돌 관리 ===
    bool    m_bHitPlayer;                           // 플레이어 충돌 여부
    bool    m_bHitGround;                           // 바닥 충돌 여부
};