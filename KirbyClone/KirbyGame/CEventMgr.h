#pragma once
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include "EventDef.h"

struct EnumClassHash { template<typename T> size_t operator()(T v) const { return (size_t)v; } };

class CEventMgr {
    SINGLE(CEventMgr);
public:
    using ListenerId = size_t;

    void init() {}
    void update(); // 프레임 말에 호출

    void AddEvent(const tEvent& e) { m_queue.push_back(e); } // Enqueue alias
    void ClearQueue() { m_queue.clear(); }

    ListenerId Subscribe(EVENT_TYPE type, std::function<void(const tEvent&)> cb, int priority = 0);
    void       Unsubscribe(EVENT_TYPE type, ListenerId id);

    void ClearAll();

private:
    struct Listener { ListenerId id{}; int priority{}; std::function<void(const tEvent&)> fn; };
    void Dispatch(const tEvent& e);
    bool IsStillSubscribed(EVENT_TYPE type, ListenerId id) const;

private:
    std::unordered_map<EVENT_TYPE, std::vector<Listener>, EnumClassHash> m_listeners;
    std::vector<tEvent> m_queue;
    ListenerId m_nextId{ 0 };
    bool m_isDispatching{ false };
};
