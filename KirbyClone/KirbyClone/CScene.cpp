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
    // 활성화된 오브젝트만 업데이트
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        for (size_t j = 0; j < m_arrObj[i].size(); ++j)
        {
            if (m_arrObj[i][j]->IsActive())
            {
                m_arrObj[i][j]->Update();
            }
        }
    }
    
    // Dead 오브젝트들 정리 (비활성화만, 삭제 안함)
    DeleteDeadObjects();
}

void CScene::Render(HDC _dc)
{
    // 활성화된 오브젝트만 렌더링
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i)
    {
        for (size_t j = 0; j < m_arrObj[i].size(); ++j)
        {
            if (m_arrObj[i][j]->IsActive())
            {
                m_arrObj[i][j]->Render(_dc);
            }
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
    // 벡터에 추가
    m_arrObj[(UINT)_eType].push_back(_pObj);
}

void CScene::DeleteDeadObjects()
{
    // 오브젝트 풀 시스템: Dead 오브젝트는 삭제하지 않고 비활성화만 유지
    // 실제 삭제는 씬 전환시에만 발생 (DeleteAllObject에서)
    // 이렇게 하면 빨아들이기 중 발생하는 더블 삭제 문제를 방지할 수 있음
}