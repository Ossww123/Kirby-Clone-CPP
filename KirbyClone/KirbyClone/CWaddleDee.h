#pragma once
#include "CBasicMonster.h"

class CWaddleDee : public CBasicMonster
{
public:
    CWaddleDee();
    virtual ~CWaddleDee();

public:
    // === 가상 함수 구현 ===
    void Move() override;                   // 기본 걷기 이동

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

private:
    void ChangeDirection();                 // 방향 전환 로직
};