#pragma once
//
// Responsibility: 2D vector helpers and basic math constants/conversions.
// Non-Goals:      Matrices, geometry ops, SIMD.
// Call-Context:   Header-only; no allocations.
//

#include <cmath>
#include <numbers>

namespace engine {

    struct Vec2
    {
        float x = 0.f;
        float y = 0.f;

        Vec2 operator+ ( const Vec2& other ) const { return { x + other.x , y + other.y }; }
        Vec2 operator- ( const Vec2& other ) const { return { x - other.x , y - other.y }; }

        Vec2& operator+= ( const Vec2& other ) { x += other.x; y += other.y; return *this; }
        Vec2& operator-= ( const Vec2& other ) { x -= other.x; y -= other.y; return *this; }

        float LengthSq ( ) const { return x * x + y * y; }
        float Length ( ) const { return std::sqrt ( LengthSq ( ) ); }

        Vec2 Normalized ( float eps = 1e-6f ) const
        {
            const float l = Length ( );
            return ( l <= eps ) ? Vec2{ 0.f , 0.f } : Vec2{ x / l , y / l };
        }
    };

    namespace math
    {
        inline constexpr float PI = std::numbers::pi_v<float>;
        inline constexpr float TAU = 2.0f * PI;

        inline constexpr float Deg2Rad ( float deg ) { return deg * ( PI / 180.0f ); }
        inline constexpr float Rad2Deg ( float rad ) { return rad * ( 180.0f / PI ); }
    }

} // namespace engine
