#include "pch.h"
#include "CScene.h"
#include "CObject.h"
#include "CMonster.h"
#include "CUIMgr.h"
#include "CFadeEffect.h"

CScene::CScene()
    : m_bPaused(false)
{
}

CScene::~CScene()
{
    // 씬이 소멸할 때 관리하던 모든 오브젝트 삭제
    DeleteAllObject();
}

void CScene::Update()
{
    // 카피능력 연출 중에는 게임 일시정지 (플레이어만 업데이트)
    bool bAbilityPresentation = CFadeEffect::GetInst()->IsPausingGame();
    
    // 일시정지 상태거나 카피능력 연출 중이면 업데이트 제한
    if (m_bPaused || bAbilityPresentation)
    {
        // 플레이어만 업데이트 (연출용 애니메이션 재생)
        for (size_t j = 0; j < m_arrObj[(UINT)GROUP_TYPE::PLAYER].size(); ++j)
        {
            if (m_arrObj[(UINT)GROUP_TYPE::PLAYER][j]->IsActive())
            {
                m_arrObj[(UINT)GROUP_TYPE::PLAYER][j]->Update();
            }
        }
        
        // 일시정지 중에도 보스는 격파 시퀀스를 위해 업데이트
        if (m_bPaused)
        {
            for (size_t j = 0; j < m_arrObj[(UINT)GROUP_TYPE::MONSTER].size(); ++j)
            {
                CObject* pObj = m_arrObj[(UINT)GROUP_TYPE::MONSTER][j];
                if (pObj->IsActive())
                {
                    CMonster* pMonster = dynamic_cast<CMonster*>(pObj);
                    if (pMonster && pMonster->IsBoss())
                    {
                        pObj->Update();
                    }
                }
            }
        }
        
        return;
    }

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
    // 커스텀 렌더링 순서: 투사체를 먼저 렌더링하여 배경에 배치
    GROUP_TYPE renderOrder[] = {
        GROUP_TYPE::DEFAULT,
        GROUP_TYPE::TILE,
        GROUP_TYPE::PROJ_PLAYER,    // 투사체들을 먼저 렌더링
        GROUP_TYPE::PROJ_MONSTER,
        GROUP_TYPE::PLAYER,         // 플레이어/몬스터를 나중에 렌더링
        GROUP_TYPE::MONSTER,
        GROUP_TYPE::ITEM,
        GROUP_TYPE::SPECIAL,
        GROUP_TYPE::EFFECT
    };

    // 지정된 순서대로 렌더링
    for (GROUP_TYPE groupType : renderOrder)
    {
        UINT groupIndex = (UINT)groupType;
        if (groupIndex < (UINT)GROUP_TYPE::END)
        {
            for (size_t j = 0; j < m_arrObj[groupIndex].size(); ++j)
            {
                if (m_arrObj[groupIndex][j]->IsActive())
                {
                    m_arrObj[groupIndex][j]->Render(_dc);
                }
            }
        }
    }

    // UI 그룹이 있다면 별도로 렌더링
    UINT uiIndex = (UINT)GROUP_TYPE::UI;
    if (uiIndex < (UINT)GROUP_TYPE::END)
    {
        for (size_t j = 0; j < m_arrObj[uiIndex].size(); ++j)
        {
            if (m_arrObj[uiIndex][j]->IsActive())
            {
                m_arrObj[uiIndex][j]->Render(_dc);
            }
        }
    }

    // UI는 항상 최상위에 렌더링
    CUIMgr::GetInst()->RenderGameUI(_dc);
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