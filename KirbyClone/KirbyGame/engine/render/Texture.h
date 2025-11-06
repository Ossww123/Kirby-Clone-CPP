#pragma once
//
// Responsibility: Lightweight 2D texture handle (SRV + size).
// Non-Goals: Lifetime policies beyond shared COM ptr, upload APIs.
// Call-Context: Header-only POD.
//

#include <wrl/client.h>

// Forward decl only (no d3d headers here)
struct ID3D11ShaderResourceView;

namespace engine {

    // D3D11 SRV + width/height (pixels)
    struct Tex2D {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        int width = 0;
        int height = 0;
    };

} // namespace engine
