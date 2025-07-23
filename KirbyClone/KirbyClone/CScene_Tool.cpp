#include "pch.h"
#include "CScene_Tool.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"

CScene_Tool::CScene_Tool()
{
}

CScene_Tool::~CScene_Tool()
{
}

void CScene_Tool::Enter()
{
    // 여러 몬스터 배치 테스트

    // 몬스터 여러 마리 생성
    for (int i = 0; i < 5; ++i)
    {
        CMonster* pMonster = new CMonster;
        pMonster->SetPos(Vec2(200.f + i * 200.f, 300.f + i * 50.f));
        pMonster->SetScale(Vec2(50.f, 50.f));

        AddObject(pMonster);
    }

    // 플레이어도 하나 추가 (테스트용)
    CPlayer* pPlayer = new CPlayer;
    pPlayer->SetPos(Vec2(640.f, 600.f));
    pPlayer->SetScale(Vec2(100.f, 100.f));

    AddObject(pPlayer);
}

void CScene_Tool::Exit()
{
    DeleteAllObject();  // 부모 클래스의 함수 호출
}