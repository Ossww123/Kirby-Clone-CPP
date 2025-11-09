#pragma once
//
// Responsibility: Lightweight 2D texture handle (SRV + size).
// Non-Goals: Lifetime policies beyond shared COM ptr, upload APIs.
// Call-Context: Header-only POD.
//

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>          // ComPtr<ID3D11ShaderResourceView> needs complete type (Release)
#include <wrl/client.h>

namespace engine {

    // D3D11 SRV + width/height (pixels)
    struct Tex2D {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        int width = 0;
        int height = 0;
    };

} // namespace engine
