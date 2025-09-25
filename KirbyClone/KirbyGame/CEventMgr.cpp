#include "gamePCH.h"
#include "CEventMgr.h"

// ===== SINGLE 매크로용 ctor/dtor =====
CEventMgr::CEventMgr() = default;
CEventMgr::~CEventMgr() = default;

void CEventMgr::init() {}

// 프레임 말에 호출: 큐 스왑 → 안전 처리
void CEventMgr::update()
{
    std::vector<tEvent> local;
    local.swap(m_queue);

    for (const auto& e : local) {
        Dispatch(e);
    }
}

CEventMgr::ListenerId
CEventMgr::Subscribe(EVENT_TYPE type, std::function<void(const tEvent&)> cb, int priority)
{
    auto& vec = m_listeners[type];
    const ListenerId id = ++m_nextId;

    vec.push_back(Listener{ id, priority, std::move(cb) });

    // 우선순위 큰 값 우선
    std::stable_sort(vec.begin(), vec.end(),
        [](const Listener& a, const Listener& b) {
            return a.priority > b.priority;
        });

    return id;
}

void CEventMgr::Unsubscribe(EVENT_TYPE type, ListenerId id)
{
    auto it = m_listeners.find(type);
    if (it == m_listeners.end()) return;

    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [&](const Listener& L) { return L.id == id; }),
        vec.end());
}

void CEventMgr::ClearAll()
{
    m_listeners.clear();
    m_queue.clear();
    m_nextId = 0;
}

void CEventMgr::Dispatch(const tEvent& e)
{
    auto it = m_listeners.find(e.eType);
    if (it == m_listeners.end()) return;

    // 발행 중 구독/해지 안전성을 위해 스냅샷 사용
    const auto snapshot = it->second;

    m_isDispatching = true;
    for (const auto& L : snapshot) {
        // 발행 도중 해지되었으면 스킵
        if (!IsStillSubscribed(e.eType, L.id)) continue;
        L.fn(e);
    }
    m_isDispatching = false;
}

bool CEventMgr::IsStillSubscribed(EVENT_TYPE type, ListenerId id) const
{
    auto it = m_listeners.find(type);
    if (it == m_listeners.end()) return false;
    const auto& vec = it->second;
    return std::any_of(vec.begin(), vec.end(), [&](const Listener& L) { return L.id == id; });
}
