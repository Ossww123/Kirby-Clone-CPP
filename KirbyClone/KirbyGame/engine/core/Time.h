// engine/core/Time.h
//
// Role: Frame timing helper (variable dt + fixed-step accumulator and FPS).
// Note: Main-thread only; no scheduling/profiling or multithread timing.
//

#pragma once

#include <memory>

namespace engine {

    class Time
    {
    public:
        static constexpr double FIXED_DT = 1.0 / 60.0;

        Time ( );                     // default ctor
        ~Time ( );                    // out-of-line dtor

        Time ( const Time& ) = delete;
        Time& operator=( const Time& ) = delete;
        Time ( Time&& ) noexcept;      // movable
        Time& operator=( Time&& ) noexcept;

        void Init ( );             // reset clocks
        void TickFrame ( );        // call once per frame

        bool   ShouldFixedUpdate ( ) const;
        void   ConsumeFixedStep ( );
        void   CapAccumulator ( int maxSteps = 5 );

        double DeltaTime ( ) const { return m_frameDT; } // variable dt (sec)
        double FixedDelta ( ) const { return FIXED_DT; } // fixed dt (sec)
        int    FPS ( ) const { return m_fps; }

    private:
        struct Impl;                          // platform state (hidden)
        std::unique_ptr<Impl> m_impl;         // no allocations in Tick/Update

        double m_frameDT = 0.0;
        double m_accumulator = 0.0;

        int    m_fps = 0;
        int    m_fpsCounter = 0;
        double m_fpsTimeAcc = 0.0;
    };

} // namespace engine
