#pragma once

class CObject;

class CScene
{
private:
    vector<CObject*> m_arrObj[(UINT)GROUP_TYPE::END];  // 그룹별로 오브젝트 관리

public:
    virtual void Update();              // 모든 오브젝트 업데이트
    virtual void Render(HDC _dc);       // 모든 오브젝트 렌더링

    // 씬에 진입할 때와 나갈 때 호출될 가상 함수
    virtual void Enter() = 0;           // 순수 가상 함수 - 자식 클래스에서 반드시 구현
    virtual void Exit() = 0;            // 순수 가상 함수 - 자식 클래스에서 반드시 구현

protected:
    void DeleteAllObject();             // 모든 오브젝트 삭제

public:
    void AddObject(CObject* _pObj, GROUP_TYPE _eType);
    const vector<CObject*>& GetGroupObject(GROUP_TYPE _eType) { return m_arrObj[(UINT)_eType]; }

public:
    CScene();
    virtual ~CScene();
};