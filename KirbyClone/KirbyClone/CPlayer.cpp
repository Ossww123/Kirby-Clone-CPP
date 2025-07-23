#include "pch.h"
#include "CPlayer.h"

#include "CKeyMgr.h"
#include "CTimeMgr.h"

CPlayer::CPlayer()
{
}

CPlayer::~CPlayer()
{
}

void CPlayer::Update()
{
    Vec2 vPos = GetPos();

    // 키 입력에 따른 이동
    if (KEY_HOLD(KEY::LEFT))
    {
        vPos.x -= 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::RIGHT))
    {
        vPos.x += 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::UP))
    {
        vPos.y -= 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    if (KEY_HOLD(KEY::DOWN))
    {
        vPos.y += 200.f * CTimeMgr::GetInst()->GetfDT();
    }

    // 스페이스바로 중앙 이동
    if (KEY_TAP(KEY::SPACE))
    {
        vPos = Vec2(640.f, 400.f);
    }

    SetPos(vPos);
}