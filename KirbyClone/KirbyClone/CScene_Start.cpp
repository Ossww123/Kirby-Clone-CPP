#include "pch.h"
#include "CScene_Start.h"

#include "CObject.h"
#include "CPlayer.h"

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
    pPlayer->SetPos(Vec2(640.f, 400.f));    // 화면 중앙
    pPlayer->SetScale(Vec2(100.f, 100.f));  // 100x100 크기

    // 씬에 플레이어 추가
    AddObject(pPlayer);
}

void CScene_Start::Exit()
{
    // 부모 클래스(CScene)의 소멸자가 오브젝트들을 자동으로 정리하므로
    // 여기서는 특별히 할 일이 없음
}