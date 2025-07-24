#pragma once

class CObject;

class CEventMgr
{
    SINGLE(CEventMgr);

private:
    vector<tEvent>  m_vecEvent;         // 현재 프레임에 발생한 이벤트들
    vector<CObject*> m_vecGarbage;      // 삭제 예정 오브젝트들

public:
    void init();
    void update();  // 모든 이벤트 처리 (프레임 마지막에 호출)

    void AddEvent(const tEvent& _event) { m_vecEvent.push_back(_event); }

private:
    void Execute(tEvent& _event);       // 개별 이벤트 실행
    void ClearGarbageObject();          // 삭제 예정 오브젝트들 정리
};