#include "gamePCH.h"
#include "CScene.h"
#include "CObject.h"
#include "CMonster.h"
#include "CFadeEffect.h"

#include <algorithm>

CScene::CScene() : m_bPaused(false) {}
CScene::~CScene() { DeleteAllObject(); }

void CScene::Update()
{
    //if (m_bPaused) {
    //    // (필요시) 플레이어/보스만 업데이트하는 특수 로직 유지 가능
    //    for (auto* obj : m_arrObj[(UINT)GROUP_TYPE::PLAYER])
    //        if (obj->IsAlive()) obj->Update();
    //    for (auto* obj : m_arrObj[(UINT)GROUP_TYPE::MONSTER]) {
    //        if (!obj->IsAlive()) continue;
    //        if (auto* m = dynamic_cast<CMonster*>(obj))
    //            if (m->IsBoss()) obj->Update();
    //    }
    //    return;
    //}

    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i) {
        for (auto* obj : m_arrObj[i]) {
            if (obj->IsAlive()) obj->Update();
        }
    }

    DeleteDeadObjects();
}

void CScene::Render(HDC _dc)
{
    // 렌더 순서: (기존 유지) + UI 그룹은 마지막에
    GROUP_TYPE renderOrder[] = {
        GROUP_TYPE::DEFAULT,
        GROUP_TYPE::TILE,
        GROUP_TYPE::PROJ_PLAYER,
        GROUP_TYPE::PROJ_MONSTER,
        GROUP_TYPE::PLAYER,
        GROUP_TYPE::MONSTER,
        GROUP_TYPE::ITEM,
        GROUP_TYPE::SPECIAL,
        GROUP_TYPE::EFFECT
    };

    for (GROUP_TYPE g : renderOrder) {
        const UINT gi = (UINT)g;
        if (gi >= (UINT)GROUP_TYPE::END) continue;
        for (auto* obj : m_arrObj[gi]) {
            if (obj->IsAlive()) obj->Render(_dc);
        }
    }

    // UI 그룹은 항상 최상단
    const UINT uiIndex = (UINT)GROUP_TYPE::UI;
    if (uiIndex < (UINT)GROUP_TYPE::END) {
        for (auto* obj : m_arrObj[uiIndex]) {
            if (obj->IsAlive()) obj->Render(_dc);
        }
    }
}

void CScene::DeleteAllObject()
{
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i) {
        for (auto* obj : m_arrObj[i]) delete obj;
        m_arrObj[i].clear();
    }
}

void CScene::AddObject(CObject* _pObj, GROUP_TYPE _eType)
{
    // 그룹에 맞게 삽입
    m_arrObj[(UINT)_eType].push_back(_pObj);

    //   캐리오버 호환: 오브젝트 자신의 그룹 기록도 맞춰 둠
    //   (DetachObject가 obj->GetGroupType()를 우선 사용한다면 필수)
    _pObj->SetGroup(_eType);

    // 씬에 등록되는 시점에 1회 초기화 보장
    _pObj->InitOnce();
}

void CScene::DeleteDeadObjects()
{
    // 현재 정책: Dead는 씬 전환 시에만 delete. 여기서는 미삭제(풀)
    // 필요 시 Dead만 골라 erase/remove_if로 빠르게 정리하도록 확장 가능.
}

bool CScene::DetachObject(CObject* obj)
{
    if (!obj) return false;

    // 1) obj가 들고 있는 그룹에서 먼저 시도
    const GROUP_TYPE g = obj->GetGroup();
    if ((UINT)g < (UINT)GROUP_TYPE::END) {
        auto& v = m_arrObj[(UINT)g];
        auto it = std::find(v.begin(), v.end(), obj);
        if (it != v.end()) { v.erase(it); return true; }
    }

    // 2) 못 찾으면 전체 그룹 탐색 (안전장치)
    for (UINT i = 0; i < (UINT)GROUP_TYPE::END; ++i) {
        auto& v = m_arrObj[i];
        auto it = std::find(v.begin(), v.end(), obj);
        if (it != v.end()) { v.erase(it); return true; }
    }
    return false;
}
