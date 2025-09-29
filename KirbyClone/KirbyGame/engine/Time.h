#pragma once
#include <windows.h>

namespace engine {
    class Time {
    public:
        static constexpr double FIXED_DT = 1.0 / 60.0;

        void Init ( ) {
            QueryPerformanceFrequency ( &m_freq );
            QueryPerformanceCounter ( &m_prev );
        }

        // 프레임 시작 시 호출
        void TickFrame ( ) {
            LARGE_INTEGER now{};
            QueryPerformanceCounter ( &now );
            const double dt = double ( now.QuadPart - m_prev.QuadPart ) / double ( m_freq.QuadPart );
            m_prev = now;

            m_frameDT = dt;
            m_accumulator += dt;

            // FPS
            m_fpsTimeAcc += dt;
            ++m_fpsCounter;
            if ( m_fpsTimeAcc >= 1.0 ) {
                m_fps = m_fpsCounter;
                m_fpsCounter = 0;
                m_fpsTimeAcc -= 1.0;
            }
        }

        bool ShouldFixedUpdate ( ) const { return m_accumulator >= FIXED_DT; }
        void ConsumeFixedStep ( ) { m_accumulator -= FIXED_DT; }

        // getters
        double DeltaTime ( )     const { return m_frameDT; } // 가변 렌더용
        double FixedDelta ( )    const { return FIXED_DT; }  // 고정 업데이트용
        int    FPS ( )           const { return m_fps; }

    private:
        LARGE_INTEGER m_freq{};
        LARGE_INTEGER m_prev{};
        double m_frameDT{ 0.0 };
        double m_accumulator{ 0.0 };

        // fps
        int    m_fps{ 0 };
        int    m_fpsCounter{ 0 };
        double m_fpsTimeAcc{ 0.0 };
    };
} // namespace engine
