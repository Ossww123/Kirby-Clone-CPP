#pragma once
//
// Responsibility: Batched 2D sprite rendering on D3D11 (sorting + state bucketing).
// Non-Goals: Depth/MSAA, instancing, texture atlasing, command buffering across frames.
// Call-Context: Main thread; non-PMA RGBA pipeline assumed (SRC_ALPHA / INV_SRC_ALPHA).
//

#include <vector>
#include <cstdint>
#include <wrl/client.h>   // ComPtr
#include "engine/render/Texture.h"   // Tex2D (srv + size)
#include "engine/util/Types.h"       // IntRect (for gradual RECT->IntRect transition)

// Forward decls (keep header focused)
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11RasterizerState;

namespace engine {

    // Presets
    enum class BlendMode : std::uint8_t { Alpha = 0 , Additive = 1 };
    enum class SamplerMode : std::uint8_t { Linear = 0 , Point = 1 };

    // Vertex layout
    struct SpriteVertex {
        float    x , y;      // screen space (pixels, Y flipped by projection)
        float    u , v;      // normalized UV
        uint32_t rgba;      // 0xAARRGGBB
    };

    // Submission item (with sort key)
    struct SpriteItem {
        std::uint64_t sortKeyHi = 0;  // [z:16 | blend:8 | sampler:8 | pad:32]
        std::uint64_t seq = 0;        // submission order (stable tie-breaker)
        const Tex2D*  tex = nullptr;
        IntRect       src{ 0,0,0,0 };   // kept for backward compat (see IntRect overload)
        float         x = 0 , y = 0 , w = 0 , h = 0;
        float         rotation = 0 , originX = 0 , originY = 0;
        uint32_t      rgba = 0xFFFFFFFF;
    };

    class D3D11SpriteBatch {
    public:
        D3D11SpriteBatch ( ) = default;
        ~D3D11SpriteBatch ( );

        bool Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH );
        void OnResize ( int w , int h ) { SetProjection ( w , h ); }

        // Collect / flush
        void Begin ( );   // binds fixed pipeline (VS/PS/IL/CB/sampler/blend/rasterizer)
        void End ( );     // sort, bucket, upload, draw

        // IntRect overloads for gradual RECT->IntRect transition
        void Draw ( const Tex2D& tex ,
                  float x , float y , float w , float h ,
                  const IntRect* srcPixels ,
                  uint32_t tintRGBA = 0xFFFFFFFF ,
                  float rotation = 0.f , float originX = 0.f , float originY = 0.f );

        void Draw ( const Tex2D& tex ,
                  float x , float y , float w , float h ,
                  const IntRect* srcPixels ,
                  uint32_t tintRGBA ,
                  float rotation ,
                  float originX , float originY ,
                  int16_t zSort ,
                  BlendMode blend = BlendMode::Alpha ,
                  SamplerMode sampler = SamplerMode::Point );

        // Defaults
        void SetDefaultBlend ( BlendMode m ) { m_defaultBlend = m; }
        void SetDefaultSampler ( SamplerMode m ) { m_defaultSampler = m; }

    private:
        // Pipeline / constants
        bool CreatePipeline ( );
        void CreateStates ( );
        void SetProjection ( int w , int h );

        // Helpers
        void ensureVB ( std::size_t vertices );
        void ensureIB ( std::size_t indices );
        void flushBatches ( );

        void applyBlend ( BlendMode );
        void applySampler ( SamplerMode );
        void applyTexture ( const Tex2D* );

        static UINT D3DCompileFlagsRowMajor ( );

    private:
        // Device/Context (non-owning)
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_ctx = nullptr;

        // Shaders / IL / constants
        Microsoft::WRL::ComPtr<ID3D11VertexShader>    m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>     m_ps;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>     m_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>          m_cbProj; // 4x4

        // States
        Microsoft::WRL::ComPtr<ID3D11BlendState>      m_blendAlpha;
        Microsoft::WRL::ComPtr<ID3D11BlendState>      m_blendAdd;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>    m_sampLinear;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>    m_sampPoint;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;

        // Buffers
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vb; // DYNAMIC
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_ib; // DYNAMIC
        std::size_t m_vbCapacity = 0; // vertices
        std::size_t m_ibCapacity = 0; // indices

        // Collections
        std::vector<SpriteItem>   m_items;
        std::vector<SpriteVertex> m_vertices;
        std::vector<std::uint16_t> m_indices;

        // State cache
        const Tex2D* m_boundTex = nullptr;
        BlendMode    m_boundBlend = static_cast< BlendMode >( 0xFF );
        SamplerMode  m_boundSampler = static_cast< SamplerMode >( 0xFF );

        // Defaults
        BlendMode    m_defaultBlend = BlendMode::Alpha;
        SamplerMode  m_defaultSampler = SamplerMode::Point;

        bool      m_inBegin = false;
        std::uint64_t m_seq = 0;
    };

} // namespace engine
