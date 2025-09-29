#pragma once
#include <algorithm>

namespace engine {
	struct Vec2 { float x = 0.f; float y = 0.f; };

	inline float Clamp ( float v , float lo , float hi ) { return std::max ( lo , std::min ( v , hi ) ); }
}
