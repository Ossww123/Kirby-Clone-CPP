#pragma once
#include "CObject.h"

class CMonster : public CObject
{
private:
    Vec2    m_vCenterPos;   // 중심 위치
    float   m_fSpeed;       // 이동 속도
    float   m_fMaxDistance; // 최대 이동 거리
    int     m_iDir;         // 이동 방향 (1: 오른쪽, -1: 왼쪽)

public:
    virtual void Update();

public:
    CMonster();
    ~CMonster();
};