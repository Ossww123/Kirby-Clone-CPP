#pragma once
#include "CObject.h"

class CPlayer : public CObject
{
private:

public:
    virtual void Update();  // 플레이어의 업데이트 로직 구현

public:
    CPlayer();
    ~CPlayer();
};