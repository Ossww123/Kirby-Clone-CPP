#pragma once

class CObject;
class CPlayer;

class CEventMgr
{
    SINGLE(CEventMgr);

public:
    // === 플레이어 상태 변경 이벤트 헬퍼 함수 ===
    static void RequestPlayerStateChange(CPlayer* _pPlayer, PLAYER_STATE _eNewState);

private:
    vector<tEvent>  m_vecEvent;         // 현재 프레임에 발생한 이벤트들
    vector<CObject*> m_vecGarbage;      // 삭제 예정 오브젝트들

    // === 플레이어 상태 변경 우선순위 관리 ===
    map<CPlayer*, PLAYER_STATE> m_mapPlayerStateRequests;  // 플레이어별 최고 우선순위 상태

public:
    void init();
    void update();  // 모든 이벤트 처리 (프레임 마지막에 호출)

    void AddEvent(const tEvent& _event) { m_vecEvent.push_back(_event); }

private:
    void Execute(tEvent& _event);       // 개별 이벤트 실행
    void ClearGarbageObject();          // 삭제 예정 오브젝트들 정리

    // === 플레이어 이벤트 처리 함수 ===
    void ExecutePlayerStateChange(tEvent& _event);
    void ProcessPlayerStateRequests();   // 우선순위에 따른 상태 변경 처리
    int GetStatePriority(PLAYER_STATE _eState) const;  // 상태 우선순위 반환
};