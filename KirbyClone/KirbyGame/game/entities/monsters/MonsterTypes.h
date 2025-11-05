#pragma once
namespace game {

    // 반드시 기반형을 고정해두면 전방선언/ODR 이슈가 깔끔해짐
    enum class MonsterType : int {
        WaddleDee , WaddleDoo , HotHead , Sparky , Apple , WhispyWoods
    };

    struct SpawnSpec {
        MonsterType type = MonsterType::WaddleDee;
        float x = 0.f , y = 0.f;
        int   dir = 1;   // -1/0/+1
        int   attack = 1;
        int   move = 1;
    };

} // namespace game
