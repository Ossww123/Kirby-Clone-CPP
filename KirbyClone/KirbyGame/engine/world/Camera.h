#pragma once
//
// Responsibility: 2D camera (target follow, bounds clamp, pixel snap, shake).
// Non-Goals:      Rendering, zoom/parallax, multi-viewport.
// Call-Context:   Main thread; Update() per frame; no allocations.
//

#include <windows.h> 
#include <utility>          // std::pair
#include "engine/Math.h"    // Vec2

namespace engine {

    class Camera
    {
    public:
        void SetScreenSize ( int w , int h );
        void SetWorldRect ( float l , float t , float r , float b );
        void SetWorldRect ( const RECT& r );

        void  SetSmoothSpeed ( float k );   // e.g., 10~15
        void  SetPixelSnap ( bool on );
        void  SetLookAt ( const Vec2& p );
        void  SnapImmediate ( );

        void  StartShake ( float amplitudePx , float frequency = 8.f , float decay = 4.f );
        void  Update ( double dt );

        // screen = world - offset
        std::pair<int , int> OffsetInt ( ) const;

        Vec2 Current ( ) const { return m_cur; }
        Vec2 GetLookAt ( ) const { return m_target; }

    private:
        Vec2 ClampToBounds ( const Vec2& c ) const;

    private:
        // viewport / world
        int   m_screenW = 0 , m_screenH = 0 , m_halfW = 0 , m_halfH = 0;
        float m_worldL = 0.f , m_worldT = 0.f , m_worldR = 0.f , m_worldB = 0.f;

        // state
        Vec2  m_cur{};              // camera center (world)
        Vec2  m_target{};           // follow target (world)
        float m_smoothSpeed = 10.f; // approach rate (per second)
        bool  m_pixelSnap = true;

        // shake
        float m_shakeAmp = 0.f , m_shakeFreq = 8.f , m_shakeDecay = 4.f , m_shakeTime = 0.f;
        Vec2  m_shakeOffset{};
    };

} // namespace engine
