#pragma once
#include "CCopyMonster.h"

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

private:
    // === 웨이들 두 전용 공격 ===
    void ShootBeam();                                               // 빔 발사
    void CreateBeamProjectile();                                    // 빔 투사체 생성

private:
    bool m_bBeamFired;                                              // 빔 발사 여부
};