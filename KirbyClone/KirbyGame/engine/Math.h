#pragma once
#include <algorithm>
#include <numbers>

namespace engine {
	struct Vec2 { float x = 0.f; float y = 0.f; };

	namespace math {
		// 기본 원주율 상수
		inline constexpr float PI  = std::numbers::pi_v<float>;
		inline constexpr float TAU = 2.f * PI;
		
		// 편의 함수
		inline constexpr float Deg2Rad ( float deg ) { return deg * ( PI / 180.f ); }
		inline constexpr float Rad2Deg ( float rad ) { return rad * ( 180.f / PI ); }
		
	}
}
