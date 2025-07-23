#include "pch.h"
#include "CScene.h"
#include "CObject.h"

CScene::CScene()
{
}

CScene::~CScene()
{
    // 씬이 소멸할 때 관리하던 모든 오브젝트 삭제
    DeleteAllObject();
}

void CScene::Update()
{
    // 모든 오브젝트 업데이트
    for (size_t i = 0; i < m_vecObj.size(); ++i)
    {
        m_vecObj[i]->Update();
    }
}

void CScene::Render(HDC _dc)
{
    // 모든 오브젝트 렌더링
    for (size_t i = 0; i < m_vecObj.size(); ++i)
    {
        m_vecObj[i]->Render(_dc);
    }
}

void CScene::DeleteAllObject()
{
    // 모든 오브젝트 삭제
    for (size_t i = 0; i < m_vecObj.size(); ++i)
    {
        delete m_vecObj[i];
    }
    m_vecObj.clear();
}