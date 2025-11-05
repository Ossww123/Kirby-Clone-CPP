#pragma once
//
// Responsibility: Win32 RECT <-> engine::IntRect conversion helpers.
// Non-Goals:      Window creation, message loop, rendering.
// Call-Context:   Header-only; small inline adapters.
//

#include <windows.h>
#include "engine/util/Types.h"

namespace engine::win32 {

    inline RECT ToRECT ( const engine::IntRect& s ) noexcept
    {
        return RECT{ s.l , s.t , s.r , s.b };
    }

    inline engine::IntRect FromRECT ( const RECT& r ) noexcept
    {
        return { r.left , r.top , r.right , r.bottom };
    }

} // namespace engine::win32
