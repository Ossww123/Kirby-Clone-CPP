#pragma once
//
// Responsibility: Texture load helpers (WIC file, solid 1x1).
// Non-Goals: Streaming, DDS/KTX, mipgen, staging uploads.
// Call-Context: Main thread; returns false on failure.
//

// Forward decl to keep the header light
struct ID3D11Device;

#include "engine/render/Texture.h"

namespace engine {

    // Load from file via WIC. Produces RGBA8 (non-PMA).
    [[nodiscard]] bool LoadTextureWIC ( ID3D11Device* device , const wchar_t* path , Tex2D* out );

    // Create a 1x1 solid texture (color = 0xAARRGGBB).
    [[nodiscard]] bool CreateSolidTexture1x1 ( ID3D11Device* device , unsigned int rgba , Tex2D* out );

} // namespace engine
