#pragma once
//
// Responsibility: Single-quad sprite renderer on D3D11 (pos/uv + tint).
// Non-Goals: Sprite batching, depth/MSAA, state sorting.
// Call-Context: Main thread with a valid D3D11 immediate context.
//

#include <cstdint>
#include <memory>
#include "engine/util/Types.h"     // IntRect
#include "engine/render/Texture.h" // Tex2D (SRV + size)

// Forward decls only (keep header light)
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace engine {

    class D3D11SpriteRenderer {
    public:
        D3D11SpriteRenderer ( );
        ~D3D11SpriteRenderer ( );

        D3D11SpriteRenderer ( const D3D11SpriteRenderer& ) = delete;
        D3D11SpriteRenderer& operator=( const D3D11SpriteRenderer& ) = delete;
        D3D11SpriteRenderer ( D3D11SpriteRenderer&& ) noexcept;
        D3D11SpriteRenderer& operator=( D3D11SpriteRenderer&& ) noexcept;

        bool Initialize ( void* d3dDevice , void* d3dContext , int screenW , int screenH );
        void OnResize ( int screenW , int screenH );

        // Draw a sprite to dst rectangle in pixels; src is optional (texture pixels).
        // tintRGBA: 0xAARRGGBB (defaults to white/opaque).
        void Draw ( const Tex2D& tex ,
                  float x , float y , float w , float h ,
                  const IntRect* srcPixels = nullptr ,
                  std::uint32_t tintRGBA = 0xFFFFFFFFu );

    private:
        void SetProjection ( int w , int h );

        struct Impl;                 // hidden D3D state
        std::unique_ptr<Impl> m_impl;
    };

} // namespace engine
