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
    // 모든 그룹의 오브젝트 업데이트
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        for (size_t j = 0; j < m_arrObj[i].size(); ++j)
        {
            m_arrObj[i][j]->Update();
        }
    }
}

void CScene::Render(HDC _dc)
{
    // 모든 그룹의 오브젝트 렌더링
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        for (size_t j = 0; j < m_arrObj[i].size(); ++j)
        {
            m_arrObj[i][j]->Render(_dc);
        }
    }
}

void CScene::DeleteAllObject()
{
    // 모든 그룹의 오브젝트 삭제
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        for (size_t j = 0; j < m_arrObj[i].size(); ++j)
        {
            delete m_arrObj[i][j];
        }
        m_arrObj[i].clear();
    }
}

void CScene::AddObject(CObject* _pObj, GROUP_TYPE _eType)
{
    m_arrObj[(UINT)_eType].push_back(_pObj);
}