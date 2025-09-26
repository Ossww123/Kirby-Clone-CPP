#pragma once

#include "EventDef.h"

class CollisionEventSystem {
public:
    static void Init();
    static void Shutdown();
private:
    static size_t s_subEnter;
    static size_t s_subExit;

    static void OnEnter(const tEvent& e); // w: CCollider*, l: CCollider*
    static void OnExit(const tEvent& e); // w: CCollider*, l: CCollider*
};
