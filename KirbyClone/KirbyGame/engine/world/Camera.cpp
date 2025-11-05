#include "engine/world/Camera.h"
#include <algorithm>   // std::clamp
#include <cmath>

namespace engine {

    void Camera::SetScreenSize ( int w , int h )
    {
        m_screenW = w; m_screenH = h;
        m_halfW = w / 2; m_halfH = h / 2;
    }

    void Camera::SetWorldRect ( float l , float t , float r , float b )
    {
        m_worldL = l; m_worldT = t; m_worldR = r; m_worldB = b;
    }

    void Camera::SetWorldRect ( const RECT& r )
    {
        m_worldL = static_cast< float >( r.left );
        m_worldT = static_cast< float >( r.top );
        m_worldR = static_cast< float >( r.right );
        m_worldB = static_cast< float >( r.bottom );
    }

    void Camera::SetSmoothSpeed ( float k )
    {
        m_smoothSpeed = k;
    }

    void Camera::SetPixelSnap ( bool on )
    {
        m_pixelSnap = on;
    }

    void Camera::SetLookAt ( const Vec2& p )
    {
        m_target = p;
    }

    void Camera::SnapImmediate ( )
    {
        m_cur = ClampToBounds ( m_target );
    }

    void Camera::StartShake ( float amplitudePx , float frequency , float decay )
    {
        m_shakeAmp = amplitudePx;
        m_shakeFreq = frequency;
        m_shakeDecay = decay;
        m_shakeTime = 0.f;
    }

    void Camera::Update ( double dt )
    {
        // smooth follow
        m_cur.x += ( m_target.x - m_cur.x ) * static_cast< float >( m_smoothSpeed * dt );
        m_cur.y += ( m_target.y - m_cur.y ) * static_cast< float >( m_smoothSpeed * dt );

        // clamp to world bounds
        m_cur = ClampToBounds ( m_cur );

        // shake (exponential decay)
        if ( m_shakeAmp > 0.f )
        {
            m_shakeTime += static_cast< float >( dt );

            const float decay = std::exp ( -m_shakeDecay * m_shakeTime );
            m_shakeOffset.x = m_shakeAmp * decay *
                std::sin ( engine::math::TAU * m_shakeFreq * m_shakeTime + 0.7f );
            m_shakeOffset.y = m_shakeAmp * decay *
                std::cos ( engine::math::TAU * m_shakeFreq * m_shakeTime );

            if ( decay < 0.01f )
            {
                m_shakeAmp = 0.f;
                m_shakeOffset = {};
            }
        }
        else
        {
            m_shakeOffset = {};
        }
    }

    std::pair<int , int> Camera::OffsetInt ( ) const
    {
        float cx = m_cur.x + m_shakeOffset.x;
        float cy = m_cur.y + m_shakeOffset.y;

        if ( m_pixelSnap ) { cx = std::floor ( cx ); cy = std::floor ( cy ); }

        const int ox = static_cast< int >( std::floor ( cx ) ) - m_halfW;
        const int oy = static_cast< int >( std::floor ( cy ) ) - m_halfH;
        return { ox , oy };
    }

    Vec2 Camera::ClampToBounds ( const Vec2& c ) const
    {
        const float halfWf = static_cast< float >( m_halfW );
        const float halfHf = static_cast< float >( m_halfH );

        Vec2 r = c;

        // if world smaller than screen: lock to center
        if ( m_worldR - m_worldL <= 2.f * halfWf ) r.x = ( m_worldL + m_worldR ) * 0.5f;
        else r.x = std::clamp ( c.x , m_worldL + halfWf , m_worldR - halfWf );

        if ( m_worldB - m_worldT <= 2.f * halfHf ) r.y = ( m_worldT + m_worldB ) * 0.5f;
        else r.y = std::clamp ( c.y , m_worldT + halfHf , m_worldB - halfHf );

        return r;
    }

} // namespace engine
