#include "pch.h"
#include "CScene_Start.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"

CScene_Start::CScene_Start()
{
}

CScene_Start::~CScene_Start()
{
}

void CScene_Start::Enter()
{
    // 플레이어 생성
    CObject* pPlayer = new CPlayer;
    pPlayer->SetPos(Vec2(640.f, 400.f));
    pPlayer->SetScale(Vec2(100.f, 100.f));
    AddObject(pPlayer, GROUP_TYPE::PLAYER);  // GROUP_TYPE 추가

    // 몬스터 생성
    CMonster* pMonster = new CMonster;
    pMonster->SetPos(Vec2(300.f, 300.f));
    pMonster->SetScale(Vec2(60.f, 60.f));
    AddObject(pMonster, GROUP_TYPE::MONSTER);  // GROUP_TYPE 추가
}

void CScene_Start::Exit()
{
    DeleteAllObject();  // 부모 클래스의 함수 호출
}