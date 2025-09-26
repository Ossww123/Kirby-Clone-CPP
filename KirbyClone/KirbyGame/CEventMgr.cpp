#include "gamePCH.h"
#include "CEventMgr.h"

CEventMgr::CEventMgr() = default;
CEventMgr::~CEventMgr() = default;

void CEventMgr::update() {
    std::vector<tEvent> local; local.swap(m_queue);
    for (const auto& e : local) Dispatch(e);
}

CEventMgr::ListenerId
CEventMgr::Subscribe(EVENT_TYPE type, std::function<void(const tEvent&)> cb, int priority) {
    auto& v = m_listeners[type];
    ListenerId id = ++m_nextId;
    v.push_back({ id, priority, std::move(cb) });
    std::stable_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.priority > b.priority; });
    return id;
}

void CEventMgr::Unsubscribe(EVENT_TYPE type, ListenerId id) {
    auto it = m_listeners.find(type); if (it == m_listeners.end()) return;
    auto& v = it->second;
    v.erase(std::remove_if(v.begin(), v.end(), [&](auto& L) { return L.id == id; }), v.end());
}

void CEventMgr::ClearAll() {
    m_listeners.clear(); m_queue.clear(); m_nextId = 0;
}

void CEventMgr::Dispatch(const tEvent& e) {
    auto it = m_listeners.find(e.eType); if (it == m_listeners.end()) return;
    const auto snapshot = it->second;
    m_isDispatching = true;
    for (const auto& L : snapshot) {
        if (!IsStillSubscribed(e.eType, L.id)) continue;
        L.fn(e);
    }
    m_isDispatching = false;
}

bool CEventMgr::IsStillSubscribed(EVENT_TYPE type, ListenerId id) const {
    auto it = m_listeners.find(type); if (it == m_listeners.end()) return false;
    const auto& v = it->second;
    return std::any_of(v.begin(), v.end(), [&](auto& L) { return L.id == id; });
}
