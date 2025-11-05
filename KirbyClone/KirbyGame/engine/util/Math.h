#pragma once
#include <algorithm>
#include <numbers>
#include <cmath>

namespace engine {
	struct Vec2 {
		float x = 0.f; float y = 0.f; 

		Vec2 operator+( const Vec2 & other ) const { return { x + other.x, y + other.y }; }
		Vec2 operator-( const Vec2 & other ) const { return { x - other.x, y - other.y }; }
		Vec2 & operator+=( const Vec2 & other ) { x += other.x; y += other.y; return *this; }
		Vec2 & operator-=( const Vec2 & other ) { x -= other.x; y -= other.y; return *this; }
		
		float LengthSq ( ) const { return x * x + y * y; }
		float Length ( ) const { return std::sqrt ( LengthSq ( ) ); }
		Vec2  Normalized ( float eps = 1e-6f ) const {
			const float l = Length ( );
			return ( l <= eps ) ? Vec2{ 0.f, 0.f } : Vec2{ x / l, y / l };
		}
	};

	namespace math {
		inline constexpr float PI  = std::numbers::pi_v<float>;
		inline constexpr float TAU = 2.f * PI;
		
		inline constexpr float Deg2Rad ( float deg ) { return deg * ( PI / 180.f ); }
		inline constexpr float Rad2Deg ( float rad ) { return rad * ( 180.f / PI ); }
		
	}
}
