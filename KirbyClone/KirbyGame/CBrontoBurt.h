#pragma once
#include "CBasicMonster.h"

class CBrontoBurt : public CBasicMonster
{
public:
    CBrontoBurt();
    ~CBrontoBurt() override = default;

protected:
    // 수평 + 상하 파동 비행
    void Move() override;

    // 필요 시 애니 매핑 커스텀
    // void SetupAnimationMapping() override;

private:
    float m_fWaveTime = 0.f;  // 누적 시간
    float m_fWaveSpeed = 2.2f; // 파동 속도(라디안/초)
    float m_fWaveVelAmp = 60.f; // 수직 속도 진폭(px/s)
};
