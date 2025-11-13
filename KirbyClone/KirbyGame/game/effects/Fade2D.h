#pragma once
//
// Responsibility: Simple full-screen fade (in/out) with z-layer & blend policy.
// Non-Goals: Color grading, curves, timeline editor.
// Call-Context: Main thread; Update() is allocation-free.
//

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "engine/core/RenderSystem.h"     // RenderSystem, BlendMode, SamplerMode
#include "engine/render/Texture.h"        // Tex2D
#include "engine/platform/win32/ColorUtil.h" // RGBA8
#include "game/render/ZOrder.h"

namespace game {

    class Fade2D {
    public:
        enum class Mode { None , In , Out };

        struct Params {
            float     duration;
            uint32_t  rgb;
            int16_t   z;
            engine::BlendMode   blend;
            engine::SamplerMode sampler;

            // NOTE: we use numeric defaults (0 == Alpha / Linear) to avoid needing enum definitions here
            Params ( )
                : duration ( 0.6f )
                , rgb ( 0xFFFFFFu )
                , z ( Z::OverlayTop )
                , blend ( static_cast< engine::BlendMode >( 0 ) )     // Alpha
                , sampler ( static_cast< engine::SamplerMode >( 0 ) ) // Linear
            { }
        };

        void Reset ( ) { m_mode = Mode::None; m_t = 0.f; m_p = {}; }
        void StartIn ( const Params& p ) { m_mode = Mode::In;  m_t = 0.f; m_p = p; }
        void StartOut ( const Params& p ) { m_mode = Mode::Out; m_t = 0.f; m_p = p; }

        void Update ( double dt ) {
            if ( m_mode == Mode::None || m_p.duration <= 0.f ) return;
            m_t = std::min ( m_t + static_cast< float >( dt ) , m_p.duration );
            if ( m_t >= m_p.duration ) m_mode = Mode::None;
        }

        bool   Active ( ) const { return m_mode != Mode::None && m_p.duration > 0.f; }
        float  Alpha01 ( ) const {
            if ( !Active ( ) ) return 0.f;
            const float t = std::clamp ( m_t / std::max ( 0.0001f , m_p.duration ) , 0.f , 1.f );
            return ( m_mode == Mode::Out ) ? t : ( 1.f - t );
        }

        void Render ( engine::RenderSystem* rs , const engine::Tex2D& whiteTex , int sw , int sh ) const {
            if ( !rs || !whiteTex.srv || !Active ( ) ) return;

            const float   a01 = Alpha01 ( );
            const uint8_t a = static_cast< uint8_t >( std::lround ( a01 * 255.f ) );
            const uint8_t r = static_cast< uint8_t >( ( m_p.rgb >> 16 ) & 0xFF );
            const uint8_t g = static_cast< uint8_t >( ( m_p.rgb >> 8 ) & 0xFF );
            const uint8_t b = static_cast< uint8_t >( m_p.rgb & 0xFF );

            rs->DrawSprite (
                whiteTex ,
                0.f , 0.f , static_cast< float >( sw ) , static_cast< float >( sh ) ,
                nullptr ,
                engine::win32::RGBA8 ( r , g , b , a ) ,
                0.f , 0.f , 0.f ,
                /*z*/ m_p.z ,
                m_p.blend ,
                m_p.sampler
            );
        }

    private:
        Mode   m_mode = Mode::None;
        float  m_t = 0.f;
        Params m_p{};
    };

} // namespace game
