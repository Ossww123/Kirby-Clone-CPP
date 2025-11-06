#include "engine/core/RenderSystem.h"
#include "engine/render/D3D11SpriteBatch.h"
#include "engine/render/D3D11DebugDraw.h"
#include "engine/render/Texture.h"
#include "engine/world/Camera.h"

namespace engine {

    bool RenderSystem::Init ( IRenderer* renderer ) {
        m_renderer = renderer;
        if ( !m_renderer ) return false;

        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        if ( !m_renderer->GetD3D11Handles ( &dev , &ctx ) ) return false;

        const auto bb = m_renderer->GetBackbufferSize ( );

        m_batch = std::make_unique<D3D11SpriteBatch> ( );
        if ( !m_batch->Initialize ( dev , ctx , bb.w , bb.h ) ) return false;

        m_dbg = std::make_unique<D3D11DebugDraw> ( );
        if ( !m_dbg->Initialize ( dev , ctx , bb.w , bb.h ) ) return false;

        m_curBlend = 0xFF; m_curSampler = 0xFF;
        return true;
    }

    void RenderSystem::OnResize ( int w , int h ) {
        if ( m_batch ) m_batch->OnResize ( w , h );
        if ( m_dbg )   m_dbg->OnResize ( w , h );
    }

    void RenderSystem::Begin ( const Color& clear ) {
        if ( m_renderer ) m_renderer->BeginFrame ( clear );
        if ( m_dbg )   m_dbg->BeginFrame ( );
        if ( m_batch ) m_batch->Begin ( );

        // reset caches
        m_curBlend = 0xFF;
        m_curSampler = 0xFF;
    }

    void RenderSystem::End ( ) {
        if ( m_batch ) m_batch->End ( );
        if ( m_dbg )   m_dbg->Flush ( );
        if ( m_renderer ) m_renderer->EndFrame ( );
    }

    void RenderSystem::SetConfig ( const RenderConfig& cfg ) {
        m_cfg = cfg;
        // hook: sampler/blend switching can be applied here if needed
    }

    void RenderSystem::SetBlendMode ( uint8_t mode ) { m_curBlend = mode; }
    void RenderSystem::SetSamplerMode ( uint8_t mode ) { m_curSampler = mode; }

    std::pair<float , float> RenderSystem::ToScreen ( float wx , float wy ) const {
        int ox = 0 , oy = 0;
        if ( m_cam ) {
            auto off = m_cam->OffsetInt ( );
            ox = off.first; oy = off.second;
        }

        const auto bb = m_renderer ? m_renderer->GetBackbufferSize ( ) : BackbufferSize{ 0,0 };
        const float cx = static_cast< float >( bb.w ) * 0.5f;
        const float cy = static_cast< float >( bb.h ) * 0.5f;

        const float z = ( m_cfg.zoom > 0.f ) ? m_cfg.zoom : 1.f;
        const float sx = ( wx - ox - cx ) * z + cx;
        const float sy = ( wy - oy - cy ) * z + cy;
        return { sx, sy };
    }

    void RenderSystem::DrawSprite ( const Tex2D& tex ,
                                  float wx , float wy , float w , float h ,
                                  const IntRect* src ,
                                  uint32_t rgba ,
                                  float rotation ,
                                  float originX , float originY ,
                                  float /*z*/ , uint8_t /*sortBlend*/ )
    {
        if ( !m_batch ) return;

        // world → screen
        auto [sx , sy] = ToScreen ( wx , wy );

        // apply zoom
        const float z = ( m_cfg.zoom > 0.f ) ? m_cfg.zoom : 1.f;
        const float sw = w * z;
        const float sh = h * z;

        // SpriteBatch v2 (IntRect) path
        m_batch->Draw ( tex , sx , sy , sw , sh , src , rgba , rotation , originX , originY );
    }

    D3D11SpriteBatch& RenderSystem::Batch ( ) { return *m_batch; }
    D3D11DebugDraw& RenderSystem::Debug ( ) { return *m_dbg; }

    int RenderSystem::BackbufferWidth ( )  const { return m_renderer ? m_renderer->GetBackbufferSize ( ).w : 0; }
    int RenderSystem::BackbufferHeight ( ) const { return m_renderer ? m_renderer->GetBackbufferSize ( ).h : 0; }

} // namespace engine
