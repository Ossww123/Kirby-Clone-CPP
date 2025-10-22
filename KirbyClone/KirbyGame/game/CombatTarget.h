#pragma once
#include <windows.h>
namespace game {
    struct CombatTarget {
        int  id = -1;
        RECT aabb{ 0,0,0,0 };
        bool alive = true;
        bool isPlayer = false;
    };
}
