#pragma once
#include "CBasicMonster.h"

class CBrontoBurt : public CBasicMonster
{
public:
    CBrontoBurt();
    virtual ~CBrontoBurt();

public:
    // === 가상 함수 구현 ===
    void Move() override;                   // 사인파 비행 패턴

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

private:
    // === 비행 패턴 로직 ===
    void UpdateFlightPattern();             // 사인파 비행 패턴 업데이트

private:
    float   m_fFlightTimer;                 // 비행 패턴용 타이머
    float   m_fWaveAmplitude;               // 사인파 진폭
    float   m_fWaveFrequency;               // 사인파 주파수
    Vec2    m_vStartPos;                    // 시작 위치 (기준점)
};
