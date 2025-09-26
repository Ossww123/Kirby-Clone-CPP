#pragma once

#include "EventDef.h"

class BossSystem {
public:
    static void Init();
    static void Shutdown();
private:
    static size_t s_subBossStart;
    static size_t s_subStageClear;
    static void OnBossBattleStart(const tEvent& e); // l: CTile*
    static void OnStageClear(const tEvent& e); // l: CBoss*
};
