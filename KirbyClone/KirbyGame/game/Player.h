#pragma once
#include <windows.h>
#include "engine/Object.h"
#include "engine/Input.h"
#include "engine/Math.h"

namespace game {

    class Player final : public engine::Object {
    public:
        explicit Player ( RECT playBounds ) : m_bounds ( playBounds ) {}

        void Update ( double fixedDt , const engine::Input& input ) override {
            const float mx = input.GetAxis ( "MoveX" );
            const float my = input.GetAxis ( "MoveY" );
            m_x += mx * SPEED * static_cast< float >( fixedDt );
            m_y -= my * SPEED * static_cast< float >( fixedDt ); // 화면 Y+가 아래 → 반전

            // 경계 클램프
            const int right = m_bounds.right - static_cast< int >( m_w );
            const int bottom = m_bounds.bottom - static_cast< int >( m_h );
            if ( m_x < m_bounds.left ) m_x = static_cast< float >( m_bounds.left );
            if ( m_y < m_bounds.top )  m_y = static_cast< float >( m_bounds.top );
            if ( m_x > right )  m_x = static_cast< float >( right );
            if ( m_y > bottom ) m_y = static_cast< float >( bottom );
        }

        void Render ( HDC dc , int ox , int oy ) override {
            HBRUSH br = CreateSolidBrush ( RGB ( 255 , 180 , 64 ) );
            HGDIOBJ old = SelectObject ( dc , br );
            const int sx = static_cast< int >( m_x ) - ox;
            const int sy = static_cast< int >( m_y ) - oy;
            RoundRect ( dc , sx , sy , sx + static_cast< int >( m_w ) , sy + static_cast< int >( m_h ) , 12 , 12 );
            SelectObject ( dc , old ); DeleteObject ( br );
        }

        void SetBounds ( RECT b ) { m_bounds = b; }
        engine::Vec2 Center ( ) const { return { m_x + m_w * 0.5f, m_y + m_h * 0.5f }; }

        void GetBounds ( int& x , int& y , int& w , int& h ) const {
            x = static_cast< int >( m_x );
            y = static_cast< int >( m_y );
            w = static_cast< int >( m_w );
            h = static_cast< int >( m_h );
        }

        void SetSize ( float w , float h ) { m_w = w; m_h = h; }
        void SetPosition ( float x , float y ) { m_x = x; m_y = y; }

    private:
        static constexpr float SPEED = 180.f;
        float m_x = 100.f , m_y = 100.f;
        float m_w = 32.f , m_h = 24.f;
        RECT  m_bounds{};
    };

} // namespace game
