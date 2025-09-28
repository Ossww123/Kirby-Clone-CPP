#pragma once
#include <windows.h>
#include <array>
#include <algorithm>

namespace engine {

    struct KeyState {
        bool down = false;     // 현재 눌림
        bool pressed = false;  // 이번 프레임에 막 눌림(Edge Up->Down)
        bool released = false; // 이번 프레임에 막 뗌(Edge Down->Up)
    };

    class Input {
    public:
        void Init ( HWND hWnd ) { m_hWnd = hWnd; ClearAll ( ); }

        // 프레임 시작에 호출: 이전 상태 백업
        void BeginFrame ( ) {
            m_prevDown = m_curDown;
            // 현재 물리 상태 샘플링
            for ( int vk = 0; vk < 256; ++vk ) {
                m_curDown[ vk ] = ( GetAsyncKeyState ( vk ) & 0x8000 ) != 0;
            }
            // 에지 계산
            for ( int vk = 0; vk < 256; ++vk ) {
                KeyState& ks = m_state[ vk ];
                ks.down = m_curDown[ vk ];
                ks.pressed = ( !m_prevDown[ vk ] && m_curDown[ vk ] );
                ks.released = ( m_prevDown[ vk ] && !m_curDown[ vk ] );
            }
        }

        // 포커스 잃으면 안전하게 리셋
        void OnFocusLost ( ) { ClearAll ( ); }

        // 쿼리
        bool Down ( int vk )     const { return m_state[ vk ].down; }
        bool Pressed ( int vk )  const { return m_state[ vk ].pressed; }
        bool Released ( int vk ) const { return m_state[ vk ].released; }

    private:
        void ClearAll ( ) {
            m_prevDown.fill ( false );
            m_curDown.fill ( false );
            for ( auto& s : m_state ) s = {};
        }

        HWND m_hWnd = nullptr;
        std::array<bool , 256>    m_prevDown{};
        std::array<bool , 256>    m_curDown{};
        std::array<KeyState , 256> m_state{};
    };

} // namespace engine
