#include "engine/core/Input.h"
#include <cassert>

namespace engine {

    void Input::Init ( HWND hWnd )
    {
        m_hWnd = hWnd;
        ClearAll ( );
    }

    void Input::BeginFrame ( )
    {
        // prev snapshot
        m_prevDown = m_curDown;

        // poll VK states
        for ( int vk = 0; vk < 256; ++vk ) {
            m_curDown[ vk ] = ( GetAsyncKeyState ( vk ) & 0x8000 ) != 0;
        }

        // mouse pos/delta
        POINT p;
        GetCursorPos ( &p );
        ScreenToClient ( m_hWnd , &p );
        m_mouseDelta.x = p.x - m_mousePos.x;
        m_mouseDelta.y = p.y - m_mousePos.y;
        m_mousePos = p;

        // edges
        for ( int vk = 0; vk < 256; ++vk ) {
            KeyState& ks = m_state[ vk ];
            ks.down = m_curDown[ vk ];
            ks.pressed = ( !m_prevDown[ vk ] && m_curDown[ vk ] );
            ks.released = ( m_prevDown[ vk ] && !m_curDown[ vk ] );
        }
    }

    std::intptr_t Input::OnWndMessage ( HWND /*hWnd*/ , unsigned msg , std::uintptr_t wParam , std::intptr_t /*lParam*/ )
    {
        switch ( msg ) {
        case WM_MOUSEWHEEL: {
            const int notches = GET_WHEEL_DELTA_WPARAM ( static_cast< WPARAM >( wParam ) ) / WHEEL_DELTA; // +/-1
            m_wheelAccum += notches;
            return 0;
        }
        case WM_ACTIVATE: {
            if ( LOWORD ( static_cast< WPARAM >( wParam ) ) == WA_INACTIVE )
                OnFocusLost ( );
            return 0;
        }
        default:
            break;
        }
        return 0;
    }

    void Input::OnFocusLost ( )
    {
        ClearAll ( );
    }

    bool Input::Down ( int vk ) const
    {
        assert ( vk >= 0 && vk < static_cast< int >( m_state.size ( ) ) );
        return m_state[ static_cast< std::size_t >( vk ) ].down;
    }

    bool Input::Pressed ( int vk ) const
    {
        assert ( vk >= 0 && vk < static_cast< int >( m_state.size ( ) ) );
        return m_state[ static_cast< std::size_t >( vk ) ].pressed;
    }

    bool Input::Released ( int vk ) const
    {
        assert ( vk >= 0 && vk < static_cast< int >( m_state.size ( ) ) );
        return m_state[ static_cast< std::size_t >( vk ) ].released;
    }

    int Input::ConsumeWheel ( )
    {
        int w = m_wheelAccum;
        m_wheelAccum = 0;
        return w;
    }

    void Input::BindAction ( const std::string& name , int vk )
    {
        m_actionMap[ name ].push_back ( vk );
    }

    bool Input::ActionDown ( const std::string& name ) const
    {
        auto it = m_actionMap.find ( name );
        if ( it == m_actionMap.end ( ) ) return false;
        for ( int vk : it->second ) if ( Down ( vk ) ) return true;
        return false;
    }

    bool Input::ActionPressed ( const std::string& name ) const
    {
        auto it = m_actionMap.find ( name );
        if ( it == m_actionMap.end ( ) ) return false;
        for ( int vk : it->second ) if ( Pressed ( vk ) ) return true;
        return false;
    }

    bool Input::ActionReleased ( const std::string& name ) const
    {
        auto it = m_actionMap.find ( name );
        if ( it == m_actionMap.end ( ) ) return false;
        for ( int vk : it->second ) if ( Released ( vk ) ) return true;
        return false;
    }

    void Input::BindAxis ( const std::string& name , const AxisBinding& b )
    {
        m_axisMap[ name ].push_back ( b );
    }

    float Input::Axis ( const std::string& name ) const
    {
        float v = 0.f;
        auto it = m_axisMap.find ( name );
        if ( it != m_axisMap.end ( ) ) {
            for ( const auto& b : it->second ) {
                if ( b.positiveVK && Down ( b.positiveVK ) ) v += 1.f * b.scale;
                if ( b.negativeVK && Down ( b.negativeVK ) ) v += -1.f * b.scale;
            }
        }
        if ( v > 1.f ) v = 1.f;
        if ( v < -1.f ) v = -1.f;
        return v;
    }

    void Input::ClearAll ( )
    {
        m_prevDown.fill ( false );
        m_curDown.fill ( false );
        for ( auto& s : m_state ) s = {};
        m_mousePos = { 0, 0 };
        m_mouseDelta = { 0, 0 };
        m_wheelAccum = 0;
    }

} // namespace engine
