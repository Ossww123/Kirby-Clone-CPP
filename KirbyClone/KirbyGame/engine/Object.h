#pragma once
#include <windows.h>
#include "engine/Input.h"

namespace engine {
    class Object {
    public:
        virtual ~Object ( ) = default;
        virtual void Update ( double fixedDt , const Input& input ) {}
        virtual void Render ( HDC dc , int ox , int oy ) {}
    };
} // namespace engine
