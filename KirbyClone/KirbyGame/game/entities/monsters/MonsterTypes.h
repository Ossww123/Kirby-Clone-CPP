#pragma once
//
// Responsibility: Monster ids and simple spawn spec.
// Non-Goals:      AI/behavior or factory logic.
// Call-Context:   Header-only types; used across game.
// Notes:          Fix enum underlying type for clean fwd-decl/ODR.
//
namespace game {

    enum class MonsterType : int { WaddleDee , WaddleDoo , HotHead , Sparky , Apple , WhispyWoods };

    struct SpawnSpec {
        MonsterType type = MonsterType::WaddleDee;
        float x = 0.f , y = 0.f;
        int   dir = 1;   // -1/0/+1
        int   attack = 1;
        int   move = 1;
    };

} // namespace game
