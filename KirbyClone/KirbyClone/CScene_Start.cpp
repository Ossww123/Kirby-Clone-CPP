#include "pch.h"
#include "CScene_Start.h"

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"
#include "CCore.h"
#include "CTile.h"
#include "CObjectFactory.h"
#include "CCamera.h"
#include "CCollisionMgr.h"
#include "CMonster.h"
#include "CWaddleDee.h"

CScene_Start::CScene_Start()
{
}

CScene_Start::~CScene_Start()
{
}

void CScene_Start::Update()
{
    CScene::Update();
}

void CScene_Start::Enter()
{
    // 플레이어 생성 (중앙 상단에 배치)
    CObject* pObj = new CPlayer;
    pObj->SetPos(Vec2(480.f, 200.f));
    pObj->SetScale(Vec2(64.f, 64.f));
    AddObject(pObj, GROUP_TYPE::PLAYER);

    // === 웨이들 디 몬스터들 생성 ===

    // 왼쪽에 웨이들 디 1마리
    CWaddleDee* pWaddleDee1 = new CWaddleDee;
    pWaddleDee1->SetPos(Vec2(150.f, 450.f));
    pWaddleDee1->SetScale(Vec2(64.f, 64.f));
    AddObject(pWaddleDee1, GROUP_TYPE::MONSTER);

    // 오른쪽에 웨이들 디 1마리
    CWaddleDee* pWaddleDee2 = new CWaddleDee;
    pWaddleDee2->SetPos(Vec2(750.f, 150.f));
    pWaddleDee2->SetScale(Vec2(64.f, 64.f));
    AddObject(pWaddleDee2, GROUP_TYPE::MONSTER);

    // 높은 플랫폼에 웨이들 디 1마리
    CWaddleDee* pWaddleDee3 = new CWaddleDee;
    pWaddleDee3->SetPos(Vec2(400.f, 100.f));
    pWaddleDee3->SetScale(Vec2(64.f, 64.f));
    AddObject(pWaddleDee3, GROUP_TYPE::MONSTER);

    // === 바닥 타일들 생성 ===

    // 메인 바닥 플랫폼 (중앙)
    for (int i = 0; i < 12; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(200.f + i * 64.f, 500.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 왼쪽 플랫폼 (점프 테스트용)
    for (int i = 0; i < 3; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(50.f + i * 64.f, 400.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 오른쪽 플랫폼 (점프 테스트용)
    for (int i = 0; i < 4; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(850.f + i * 64.f, 350.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 높은 플랫폼 (공중 부양 테스트용)
    for (int i = 0; i < 2; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(400.f + i * 64.f, 250.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // === 벽 타일들 생성 ===

    // 왼쪽 벽
    for (int i = 0; i < 8; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(50.f, 500.f - i * 64.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }

    // 오른쪽 벽
    for (int i = 0; i < 6; ++i)
    {
        CTile* pTile = (CTile*)CObjectFactory::CreateObject(OBJECT_TYPE::TILE_GROUND);
        pTile->SetPos(Vec2(950.f, 500.f - i * 64.f));
        AddObject(pTile, GROUP_TYPE::TILE);
    }
}

void CScene_Start::Exit()
{
    DeleteAllObject();
}