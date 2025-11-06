// TODO : 액션 / 축 매핑을 game 으로 분리하면 좋음.

#pragma once
//
// Responsibility: Poll keyboard/mouse; edge flags; simple action/axis map.
// Non-Goals:      IME, text input, rebind UI, game-level semantics.
// Call-Context:   Main thread; call BeginFrame() once per frame; no heap in hot path.
//

#include <windows.h>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>

namespace engine {

    struct KeyState
    {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    struct AxisBinding
    {
        int   positiveVK = 0;   // +1 when down
        int   negativeVK = 0;   // -1 when down
        float scale = 1.f; // multiplier
    };

    class Input
    {
    public:
        void Init ( HWND hWnd );

        // per-frame
        void BeginFrame ( );

        // optional Win32 hook (wheel/focus)
        LRESULT OnWndMessage ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );
        void    OnFocusLost ( );

        // key query
        bool Down ( int vk ) const;
        bool Pressed ( int vk ) const;
        bool Released ( int vk ) const;

        // mouse query
        POINT MousePos ( ) const { return m_mousePos; }
        POINT MouseDelta ( ) const { return m_mouseDelta; }
        int   ConsumeWheel ( ); // returns accumulated notches; resets to 0

        // action map
        void BindAction ( const std::string& name , int vk );
        bool ActionDown ( const std::string& name ) const;
        bool ActionPressed ( const std::string& name ) const;
        bool ActionReleased ( const std::string& name ) const;

        // axis map
        void  BindAxis ( const std::string& name , const AxisBinding& b );
        float GetAxis ( const std::string& name ) const;

    private:
        void ClearAll ( );

    private:
        HWND m_hWnd = nullptr;

        std::array<bool , 256>      m_prevDown{};
        std::array<bool , 256>      m_curDown{};
        std::array<KeyState , 256>  m_state{};

        // mouse
        POINT m_mousePos{ 0, 0 };
        POINT m_mouseDelta{ 0, 0 };
        int   m_wheelAccum{ 0 }; // not auto-cleared; use ConsumeWheel()

        // maps
        std::unordered_map<std::string , std::vector<int>>         m_actionMap;
        std::unordered_map<std::string , std::vector<AxisBinding>> m_axisMap;
    };

} // namespace engine
