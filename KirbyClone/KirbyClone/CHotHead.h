#pragma once
#include "CCopyMonster.h"

class CHotHead : public CCopyMonster
{
public:
    CHotHead();
    virtual ~CHotHead();

public:
    // === 가상 함수 구현 ===
    void Move() override;                                           // 걷기 + 화염 공격
    void Attack() override;                                         // 화염 공격
    COPY_ABILITY GetCopyAbility() const override { return COPY_ABILITY::FIRE; }

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

private:
    // === 핫 헤드 전용 공격 ===
    void SpitFire();                                                // 불 뿜기
    void CreateFireProjectile();                                    // 화염 투사체 생성

private:
    bool m_bFireSpat;                                               // 화염 발사 여부
    int m_iFireCount;                                               // 화염 발사 횟수
};