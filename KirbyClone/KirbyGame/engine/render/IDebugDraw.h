//
// Responsibility: Minimal, backend-agnostic debug draw interface.
// Non-Goals:      Gameplay; resource/state management; platform specifics.
// Call-Context:   Main thread during debug pass.
//
#pragma once
#include <cstdint>

namespace engine {

    using Rgba32 = std::uint32_t; // 0xAARRGGBB

    class IDebugDraw {
    public:
        virtual ~IDebugDraw ( ) = default;
        virtual void WorldLine ( int x0 , int y0 , int x1 , int y1 , int ox , int oy , Rgba32 c ) = 0;
        virtual void WorldRect ( int x , int y , int w , int h , int ox , int oy , Rgba32 c ) = 0;
    };

} // namespace engine
