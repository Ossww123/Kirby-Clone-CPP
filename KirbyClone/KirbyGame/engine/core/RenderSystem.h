#pragma once
//
// Responsibility: High-level render orchestration (frame lifecycle, camera transform,
//                 state cache, and draw facades on top of a platform IRenderer).
// Non-Goals:      Resource ownership beyond local helpers, scene graph, culling.
// Call-Context:   Main thread only. Draw calls must be between Begin()/End().
//

#include <memory>
#include <utility>
#include <cstdint>
#include "engine/render/IRenderer.h" // Color, IRenderer

namespace engine {
    // fwd to keep header light
    class D3D11SpriteBatch;
    class D3D11DebugDraw;
    class Camera;
    struct Tex2D;
    struct IntRect;

    // forward declare enums defined in D3D11SpriteBatch.h
    enum class BlendMode : std::uint8_t;
    enum class SamplerMode : std::uint8_t;

    struct RenderConfig {
        bool  usePixelSnap = true;          // handled in Camera
        float zoom = 1.0f;                  // world→screen scale
        bool  usePointSamplerForPixels = true; // reserved for policy
    };

    // High-level renderer facade
    class RenderSystem {
    public:
        RenderSystem ( ) = default;
        ~RenderSystem ( ) = default;

        // Inject platform renderer (e.g., D3D11Renderer)
        bool Init ( IRenderer* renderer );
        void OnResize ( int w , int h );

        // Frame lifecycle
        void Begin ( const Color& clear );
        void End ( );
        void Present ( );

        // Policy / state
        void SetConfig ( const RenderConfig& cfg );
        const RenderConfig& GetConfig ( ) const { return m_cfg; }

        void SetBlendMode ( uint8_t mode );    // 0: alpha, 1: additive (reserved)
        void SetSamplerMode ( uint8_t mode );  // 0: linear, 1: point   (reserved)

        // Camera
        void SetCamera ( const Camera* cam ) { m_cam = cam; }
        const Camera* GetCamera ( ) const { return m_cam; }

        // World→Screen (camera offset + zoom)
        std::pair<float , float> ToScreen ( float wx , float wy ) const;
        float Zoom ( ) const { return m_cfg.zoom; }

        // Draw facade
        void DrawSprite ( const Tex2D& tex ,
                        float wx , float wy , float w , float h ,
                        const IntRect* src = nullptr ,
                        uint32_t rgba = 0xFFFFFFFF ,
                        float rotation = 0.f ,
                        float originX = 0.f , float originY = 0.f ,
                        float z = 0.f , uint8_t sortBlend = 0 );

        // zSort/Blend/Sampler
        void DrawSprite ( const Tex2D & tex ,
                        float wx , float wy , float w , float h ,
                        const IntRect * src ,
                        uint32_t rgba ,
                        float rotation , float originX , float originY ,
                        std::int16_t zSort ,
                        BlendMode blend ,
                        SamplerMode sampler );

        // Low-level access (world/tile renderers use these directly)
        D3D11SpriteBatch& Batch ( );
        D3D11DebugDraw& Debug ( );

        // Backbuffer size
        int BackbufferWidth ( )  const;
        int BackbufferHeight ( ) const;

    private:
        IRenderer* m_renderer{};                          // non-owning
        std::unique_ptr<D3D11SpriteBatch> m_batch;        // sprite batching
        std::unique_ptr<D3D11DebugDraw>   m_dbg;          // debug lines/rects

        const Camera* m_cam{};                            // non-owning
        RenderConfig  m_cfg{};

        // lightweight state cache (reserved)
        uint8_t m_curBlend = 0xFF;
        uint8_t m_curSampler = 0xFF;
    };

} // namespace engine
