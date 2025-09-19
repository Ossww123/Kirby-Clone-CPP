#pragma once

class CObject;

class CScene
{
public:
    CScene();
    virtual ~CScene();

public:
    // === 생명주기 함수 ===
    virtual void Enter() = 0;           // 순수 가상 함수 - 자식 클래스에서 반드시 구현
    virtual void Exit() = 0;            // 순수 가상 함수 - 자식 클래스에서 반드시 구현
    virtual void Update();              // 모든 오브젝트 업데이트
    virtual void Render(HDC _dc);       // 모든 오브젝트 렌더링

public:
    // === 오브젝트 관리 ===
    void AddObject(CObject* _pObj, GROUP_TYPE _eType);
    const vector<CObject*>& GetGroupObject(GROUP_TYPE _eType) { return m_arrObj[(UINT)_eType]; }

public:
    // === 일시정지 관리 ===
    void SetPaused(bool _bPaused) { m_bPaused = _bPaused; }
    bool IsPaused() const { return m_bPaused; }

protected:
    // === 오브젝트 관리 내부 구현 ===
    void DeleteAllObject();             // 모든 오브젝트 삭제
    void DeleteDeadObjects();           // Dead 오브젝트들 정리

private:
    // === 멤버 변수 ===
    vector<CObject*> m_arrObj[(UINT)GROUP_TYPE::END];  // 그룹별로 오브젝트 관리
    bool m_bPaused;                                    // 일시정지 상태
};