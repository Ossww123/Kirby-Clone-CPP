#pragma once
#include <cmath>
#include <utility>
#include "engine/Math.h"

namespace engine {

    class Camera {
    public:
        void SetScreenSize ( int w , int h ) {
            m_screenW = w; m_screenH = h;
            m_halfW = w / 2; m_halfH = h / 2;
        }

        void SetWorldRect ( float l , float t , float r , float b ) {
            m_worldL = l; m_worldT = t; m_worldR = r; m_worldB = b;
        }

        void SetSmoothSpeed ( float k ) { m_smoothSpeed = k; }   // 10~15 권장(초당 접근률)
        void SetPixelSnap ( bool on ) { m_pixelSnap = on; }

        void SetLookAt ( const Vec2& p ) { m_target = p; }
        void SnapImmediate ( ) { m_cur = ClampToBounds ( m_target ); }

        void StartShake ( float amplitudePx , float frequency = 8.f , float decay = 4.f ) {
            m_shakeAmp = amplitudePx;
            m_shakeFreq = frequency;
            m_shakeDecay = decay;
            m_shakeTime = 0.f;
        }

        void Update ( double dt ) {
            // 스무딩
            m_cur.x += ( m_target.x - m_cur.x ) * static_cast< float >( m_smoothSpeed * dt );
            m_cur.y += ( m_target.y - m_cur.y ) * static_cast< float >( m_smoothSpeed * dt );

            // 경계 클램프
            m_cur = ClampToBounds ( m_cur );

            // 흔들림(지수 감쇠)
            if ( m_shakeAmp > 0.f ) {
                m_shakeTime += static_cast< float >( dt );
                const float decay = std::exp ( -m_shakeDecay * m_shakeTime );
                m_shakeOffset.x = m_shakeAmp * decay * std::sin ( 6.2831853f * m_shakeFreq * m_shakeTime + 0.7f );
                m_shakeOffset.y = m_shakeAmp * decay * std::cos ( 6.2831853f * m_shakeFreq * m_shakeTime );
                if ( decay < 0.01f ) { m_shakeAmp = 0.f; m_shakeOffset = {}; }
            }
            else {
                m_shakeOffset = {};
            }
        }

        // 화면 오프셋(px): 월드 → 스크린 변환에 사용 (screen = world - offset)
        std::pair<int , int> OffsetInt ( ) const {
            float cx = m_cur.x + m_shakeOffset.x;
            float cy = m_cur.y + m_shakeOffset.y;
            if ( m_pixelSnap ) { cx = std::floor ( cx ); cy = std::floor ( cy ); }
            const int ox = static_cast< int >( std::floor ( cx ) ) - m_halfW;
            const int oy = static_cast< int >( std::floor ( cy ) ) - m_halfH;
            return { ox, oy };
        }

        Vec2 Current ( ) const { return m_cur; }

    private:
        Vec2 ClampToBounds ( const Vec2& c ) const {
            const float halfWf = static_cast< float >( m_halfW );
            const float halfHf = static_cast< float >( m_halfH );
            Vec2 r = c;

            // 월드가 화면보다 작을 때의 처리(중앙 고정)
            if ( m_worldR - m_worldL <= 2.f * halfWf ) r.x = ( m_worldL + m_worldR ) * 0.5f;
            else r.x = Clamp ( c.x , m_worldL + halfWf , m_worldR - halfWf );

            if ( m_worldB - m_worldT <= 2.f * halfHf ) r.y = ( m_worldT + m_worldB ) * 0.5f;
            else r.y = Clamp ( c.y , m_worldT + halfHf , m_worldB - halfHf );

            return r;
        }

    private:
        // 화면/월드
        int   m_screenW = 0 , m_screenH = 0 , m_halfW = 0 , m_halfH = 0;
        float m_worldL = 0.f , m_worldT = 0.f , m_worldR = 0.f , m_worldB = 0.f;

        // 상태
        Vec2  m_cur{};       // 현재 카메라 중심(월드 좌표)
        Vec2  m_target{};    // 추적 목표(월드 좌표)
        float m_smoothSpeed = 10.f;
        bool  m_pixelSnap = true;

        // 흔들림
        float m_shakeAmp = 0.f , m_shakeFreq = 8.f , m_shakeDecay = 4.f , m_shakeTime = 0.f;
        Vec2  m_shakeOffset{};
    };

} // namespace engine
