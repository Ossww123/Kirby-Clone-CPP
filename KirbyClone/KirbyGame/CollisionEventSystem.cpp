#include "gamePCH.h"
#include "CollisionEventSystem.h"
#include "CEventMgr.h"
#include "CCollider.h"
#include "CObject.h"

size_t CollisionEventSystem::s_subEnter = 0;
size_t CollisionEventSystem::s_subExit = 0;

void CollisionEventSystem::Init() {
    auto& bus = *CEventMgr::GetInst();
    s_subEnter = bus.Subscribe(EVENT_TYPE::COLLISION_ENTER, &CollisionEventSystem::OnEnter, 100);
    s_subExit = bus.Subscribe(EVENT_TYPE::COLLISION_EXIT, &CollisionEventSystem::OnExit, 100);
}

void CollisionEventSystem::Shutdown() {
    auto& bus = *CEventMgr::GetInst();
    if (s_subEnter) bus.Unsubscribe(EVENT_TYPE::COLLISION_ENTER, s_subEnter), s_subEnter = 0;
    if (s_subExit) bus.Unsubscribe(EVENT_TYPE::COLLISION_EXIT, s_subExit), s_subExit = 0;
}

void CollisionEventSystem::OnEnter(const tEvent& e) {
    auto* c1 = reinterpret_cast<CCollider*>(e.wParam);
    auto* c2 = reinterpret_cast<CCollider*>(e.lParam);
    if (!c1 || !c2) return;

    if (auto* o1 = c1->GetOwner()) o1->OnCollisionEnter(c2);
    if (auto* o2 = c2->GetOwner()) o2->OnCollisionEnter(c1);
}

void CollisionEventSystem::OnExit(const tEvent& e) {
    auto* c1 = reinterpret_cast<CCollider*>(e.wParam);
    auto* c2 = reinterpret_cast<CCollider*>(e.lParam);
    if (!c1 || !c2) return;

    if (auto* o1 = c1->GetOwner()) o1->OnCollisionExit(c2);
    if (auto* o2 = c2->GetOwner()) o2->OnCollisionExit(c1);
}
