#pragma once
#include "CBasicMonster.h"

class CSparky : public CBasicMonster
{
public:
    CSparky();
    ~CSparky() override = default;

protected:
    // 필요 시 애니 매핑을 커스텀하려면 열어서 사용
    // void SetupAnimationMapping() override;
};
