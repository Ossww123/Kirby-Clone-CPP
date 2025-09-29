#pragma once
#include <windows.h>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

// ** 나중에 필요한 키만 골라서 최적화 가능 **

namespace engine {

    struct KeyState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    struct AxisBinding {
        int   positiveVK = 0;   // 누르면 +1
        int   negativeVK = 0;   // 누르면 -1
        float scale = 1.f; // 축 크기 배율
    };

    class Input {
    public:
        void Init ( HWND hWnd ) { m_hWnd = hWnd; ClearAll ( ); }

        // 프레임 시작에 호출
        void BeginFrame ( ) {
            // 이전 상태 백업
            m_prevDown = m_curDown;

            // 키/마우스 버튼 상태 폴링
            for ( int vk = 0; vk < 256; ++vk ) {
                m_curDown[ vk ] = ( GetAsyncKeyState ( vk ) & 0x8000 ) != 0;
            }

            // 마우스 좌표/델타
            POINT p; GetCursorPos ( &p );
            ScreenToClient ( m_hWnd , &p );
            m_mouseDelta.x = p.x - m_mousePos.x;
            m_mouseDelta.y = p.y - m_mousePos.y;
            m_mousePos = p;

            // 에지 계산
            for ( int vk = 0; vk < 256; ++vk ) {
                KeyState& ks = m_state[ vk ];
                ks.down = m_curDown[ vk ];
                ks.pressed = ( !m_prevDown[ vk ] && m_curDown[ vk ] );
                ks.released = ( m_prevDown[ vk ] && !m_curDown[ vk ] );
            }
        }

        // 윈도우 메시지 훅(휠 등)
        LRESULT OnWndMessage ( HWND , UINT msg , WPARAM wParam , LPARAM lParam ) {
            switch ( msg ) {
            case WM_MOUSEWHEEL:
                m_wheelAccum += GET_WHEEL_DELTA_WPARAM ( wParam ) / WHEEL_DELTA; // notch 단위(+/-1)
                return 0;
            case WM_ACTIVATE:
                if ( LOWORD ( wParam ) == WA_INACTIVE ) { OnFocusLost ( ); }
                return 0;
            default: break;
            }
            return 0; // 우리가 처리 안 한 메시지는 0 반환 (WndProc에서 DefWindowProc로 넘기면 됨)
        }

        void OnFocusLost ( ) { ClearAll ( ); }

        // === 키 쿼리 ===
        bool Down ( int vk )     const { return m_state[ vk ].down; }
        bool Pressed ( int vk )  const { return m_state[ vk ].pressed; }
        bool Released ( int vk ) const { return m_state[ vk ].released; }

        // === 마우스 쿼리 ===
        POINT MousePos ( )   const { return m_mousePos; }
        POINT MouseDelta ( ) const { return m_mouseDelta; }
        int   ConsumeWheel ( ) { int w = m_wheelAccum; m_wheelAccum = 0; return w; }

        // === 액션 매핑 ===
        void BindAction ( const std::string& name , int vk ) {
            m_actionMap[ name ].push_back ( vk );
        }
        bool ActionDown ( const std::string& name ) const {
            auto it = m_actionMap.find ( name );
            if ( it == m_actionMap.end ( ) ) return false;
            for ( int vk : it->second ) if ( Down ( vk ) ) return true;
            return false;
        }
        bool ActionPressed ( const std::string& name ) const {
            auto it = m_actionMap.find ( name );
            if ( it == m_actionMap.end ( ) ) return false;
            for ( int vk : it->second ) if ( Pressed ( vk ) ) return true;
            return false;
        }
        bool ActionReleased ( const std::string& name ) const {
            auto it = m_actionMap.find ( name );
            if ( it == m_actionMap.end ( ) ) return false;
            for ( int vk : it->second ) if ( Released ( vk ) ) return true;
            return false;
        }

        // === 축(Axis) 매핑 ===
        void BindAxis ( const std::string& name , const AxisBinding& b ) {
            m_axisMap[ name ].push_back ( b );
        }
        float GetAxis ( const std::string& name ) const {
            float v = 0.f;
            auto it = m_axisMap.find ( name );
            if ( it != m_axisMap.end ( ) ) {
                for ( const auto& b : it->second ) {
                    if ( b.positiveVK && Down ( b.positiveVK ) ) v += 1.f * b.scale;
                    if ( b.negativeVK && Down ( b.negativeVK ) ) v += -1.f * b.scale;
                }
            }
            // (선택) v를 [-1,1]로 clamp
            if ( v > 1.f ) v = 1.f;
            if ( v < -1.f ) v = -1.f;
            return v;
        }

    private:
        void ClearAll ( ) {
            m_prevDown.fill ( false );
            m_curDown.fill ( false );
            for ( auto& s : m_state ) s = {};
            m_mousePos = { 0,0 };
            m_mouseDelta = { 0,0 };
            m_wheelAccum = 0;
        }

        HWND m_hWnd = nullptr;

        std::array<bool , 256>    m_prevDown{};
        std::array<bool , 256>    m_curDown{};
        std::array<KeyState , 256> m_state{};

        // 마우스
        POINT m_mousePos{ 0,0 };
        POINT m_mouseDelta{ 0,0 };
        int   m_wheelAccum{ 0 }; // BeginFrame에서 자동 리셋하지 않음 → UI가 꺼내쓸 때 Consume

        // 매핑
        std::unordered_map<std::string , std::vector<int>>           m_actionMap;
        std::unordered_map<std::string , std::vector<AxisBinding>>   m_axisMap;
    };

} // namespace engine
