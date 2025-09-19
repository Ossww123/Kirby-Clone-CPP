#pragma once
#include "CCopyMonster.h"

class CProjectile;

class CWaddleDoo : public CCopyMonster
{
public:
    CWaddleDoo();
    virtual ~CWaddleDoo();

public:
    // === 가상 함수 구현 ===
    void Move() override;                                           // 걷기 + 공격 타이밍
    void Attack() override;                                         // 빔 공격
    COPY_ABILITY GetCopyAbility() const override { return COPY_ABILITY::BEAM; }

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;
    
    // === 상태 업데이트 오버라이드 ===
    void UpdateAttackReady() override;
    void UpdateAttack() override;

private:
    // === 웨이들 두 전용 공격 ===
    void ShootBeam();                                               // 빔 투사체 발사
    void CreateBeamProjectile();                                    // 빔 투사체 생성
    void UpdateBeamSweep();                                         // 빔 쓸어내리기 업데이트
    void ClearBeamProjectiles();                                    // 빔 투사체들 정리

private:
    bool m_bBeamFired;                                              // 빔 발사 여부
    vector<CProjectile*> m_vecBeamProjectiles;                     // 빔 띠 투사체들
    float m_fBeamSweepTimer;                                        // 빔 쓸기 타이머
    float m_fCurrentBeamAngle;                                      // 현재 빔 각도
    int m_iBeamSweepStep;                                           // 현재 쓸기 단계
};