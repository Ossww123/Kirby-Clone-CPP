#include "engine/core/Time.h"
#include <windows.h>

namespace engine {

    struct Time::Impl
    {
        LARGE_INTEGER freq{};
        LARGE_INTEGER prev{};
    };

    Time::Time ( ) = default;
    Time::~Time ( ) = default;
    Time::Time ( Time&& ) noexcept = default;
    Time& Time::operator=( Time&& ) noexcept = default;

    void Time::Init ( )
    {
        m_impl = std::make_unique<Impl> ( );
        QueryPerformanceFrequency ( &m_impl->freq );
        QueryPerformanceCounter ( &m_impl->prev );

        m_frameDT = 0.0;
        m_accumulator = 0.0;
        m_fps = 0;
        m_fpsCounter = 0;
        m_fpsTimeAcc = 0.0;
    }

    void Time::TickFrame ( )
    {
        LARGE_INTEGER now{};
        QueryPerformanceCounter ( &now );

        const double dt =
            static_cast< double >( now.QuadPart - m_impl->prev.QuadPart ) /
            static_cast< double >( m_impl->freq.QuadPart );

        m_impl->prev = now;

        m_frameDT = dt;
        m_accumulator += dt;

        m_fpsTimeAcc += dt;
        ++m_fpsCounter;
        if ( m_fpsTimeAcc >= 1.0 ) {
            m_fps = m_fpsCounter;
            m_fpsCounter = 0;
            m_fpsTimeAcc -= 1.0;
        }
    }

    bool Time::ShouldFixedUpdate ( ) const
    {
        return m_accumulator >= FIXED_DT;
    }

    void Time::ConsumeFixedStep ( )
    {
        m_accumulator -= FIXED_DT;
    }

    void Time::CapAccumulator ( int maxSteps )
    {
        const double cap = FIXED_DT * static_cast< double >( maxSteps );
        if ( m_accumulator > cap ) m_accumulator = cap;
    }

} // namespace engine
