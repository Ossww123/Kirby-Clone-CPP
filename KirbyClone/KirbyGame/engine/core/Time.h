#pragma once
//
// Responsibility: Frame timing (variable dt) + fixed-step accumulator, FPS.
// Non-Goals:      Scheduling, profiling, multithread timing.
// Call-Context:   Main thread only.
//

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
