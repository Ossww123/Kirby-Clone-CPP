#pragma once
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cstdint>

// enum class 해시
struct EnumClassHash {
    template<typename T> size_t operator()(T v) const { return static_cast<size_t>(v); }
};

class CEventMgr
{
    SINGLE(CEventMgr);  // GetInst(), private ctor/dtor

public:
    using ListenerId = size_t;

    void init();     // 선택: 필요 없으면 비워둠
    void update();   // 프레임 말에 호출 → 큐에 쌓인 이벤트 일괄 디스패치

    // === 이벤트 큐 ===
    void AddEvent(const tEvent& e) { m_queue.push_back(e); }             // 기존 호환
    void Enqueue(const tEvent& e) { m_queue.push_back(e); }             // 별칭
    void ClearQueue() { m_queue.clear(); }

    // === 구독/해지 ===
    ListenerId Subscribe(EVENT_TYPE type, std::function<void(const tEvent&)> cb, int priority = 0);
    void       Unsubscribe(EVENT_TYPE type, ListenerId id);

    // 전부 초기화(테스트/씬전환 등에서 필요하면 사용)
    void ClearAll();

    // 복사/이동 금지
    CEventMgr(const CEventMgr&) = delete;
    CEventMgr& operator=(const CEventMgr&) = delete;

private:
    struct Listener {
        ListenerId id{};
        int        priority{};
        std::function<void(const tEvent&)> fn;
    };

    void Dispatch(const tEvent& e);
    bool IsStillSubscribed(EVENT_TYPE type, ListenerId id) const;

private:
    std::unordered_map<EVENT_TYPE, std::vector<Listener>, EnumClassHash> m_listeners;
    std::vector<tEvent> m_queue;
    ListenerId m_nextId{ 0 };
    bool m_isDispatching{ false };
};
