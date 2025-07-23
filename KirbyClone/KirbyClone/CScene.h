#pragma once

class CObject;

class CScene
{
private:
    vector<CObject*> m_vecObj;  // 이 씬이 관리하는 오브젝트들

public:
    void Update();              // 모든 오브젝트 업데이트
    void Render(HDC _dc);       // 모든 오브젝트 렌더링

    // 씬에 진입할 때와 나갈 때 호출될 가상 함수
    virtual void Enter() = 0;   // 순수 가상 함수 - 자식 클래스에서 반드시 구현
    virtual void Exit() = 0;

protected:
    void DeleteAllObject();     // 모든 오브젝트 삭제

public:
    void AddObject(CObject* _pObj) { m_vecObj.push_back(_pObj); }

public:
    CScene();
    virtual ~CScene();
};