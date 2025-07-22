#pragma once

enum class KEY_STATE
{
    NONE,   // 이전에도 안눌림, 지금도 안눌림
    TAP,    // 이전에 안눌림, 지금 눌림
    HOLD,   // 이전에도 눌림, 지금도 눌림
    AWAY,   // 이전에 눌림, 지금 안눌림
};

enum class KEY
{
    LEFT,
    RIGHT,
    UP,
    DOWN,

    Q, W, E, R, T, Y,
    A, S, D, F, G, H,
    Z, X, C, V, B,

    SPACE,
    ENTER,
    ESC,

    LAST,  // enum의 끝
};