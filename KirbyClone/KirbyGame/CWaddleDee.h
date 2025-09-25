#pragma once
#include "CBasicMonster.h"

class CWaddleDee : public CBasicMonster
{
public:
    CWaddleDee();
    ~CWaddleDee() override = default;

protected:
    // 애니 이름이 시트와 다를 때만 필요 시 오버라이드
    // void SetupAnimationMapping() override;
};
