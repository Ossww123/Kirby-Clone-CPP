#pragma once
class GameFlowSystem {
public:
    static void Init();
    static void Shutdown();
private:
    static size_t s_subGameOver;
    static size_t s_subFadeComplete;

    static void OnGameOver(const tEvent& e); // w: CKirby*
    static void OnFadeComplete(const tEvent& e); // w: code(1~)
};
