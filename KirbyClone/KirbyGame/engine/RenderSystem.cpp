#include "engine/RenderSystem.h"
#include "engine/D3D11Renderer.h"  // Width/Height/Device/Context

namespace engine {

    bool RenderSystem::Init ( IRenderer* renderer )
    {
        m_renderer = renderer;
        auto* d3d = dynamic_cast< D3D11Renderer* >( renderer );
        if ( !d3d ) return false;

        m_batch = std::make_unique<D3D11SpriteBatch> ( );
        if ( !m_batch->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) ) )
            return false;

        m_dbg = std::make_unique<D3D11DebugDraw> ( );
        if ( !m_dbg->Initialize ( d3d->Device ( ) , d3d->Context ( ) , d3d->Width ( ) , d3d->Height ( ) ) )
            return false;

        m_curBlend = 0xFF;
        m_curSampler = 0xFF;
        return true;
    }

    void RenderSystem::OnResize ( int w , int h )
    {
        if ( m_batch ) m_batch->OnResize ( w , h );
        if ( m_dbg )   m_dbg->OnResize ( w , h );
    }

    void RenderSystem::Begin ( const Color& clear )
    {
        // 프레임 시작
        m_renderer->BeginFrame ( clear );

        // 디버그 & 배치 시작
        if ( m_dbg )   m_dbg->BeginFrame ( );
        if ( m_batch ) m_batch->Begin ( );

        // 상태 캐시 리셋
        m_curBlend = 0xFF;
        m_curSampler = 0xFF;
    }

    void RenderSystem::End ( )
    {
        if ( m_batch ) m_batch->End ( );
        if ( m_dbg )   m_dbg->Flush ( );
        m_renderer->EndFrame ( );
    }

    void RenderSystem::SetConfig ( const RenderConfig& cfg )
    {
        m_cfg = cfg;
        // (확장 지점) 샘플러/블렌드 상태 전환을 여기에서 즉시 반영할 수 있음.
    }

    void RenderSystem::SetBlendMode ( uint8_t mode )
    {
        // (확장) SpriteBatch가 블렌드 그룹화를 지원하면 커맨드 키에 반영
        // 지금은 캐시만 갱신해두고 Begin/End 사이에 필요 시 사용할 수 있게 둠.
        m_curBlend = mode;
    }

    void RenderSystem::SetSamplerMode ( uint8_t mode )
    {
        // (확장) 포인트/선형 샘플러 전환을 SpriteBatch 내부 혹은 Renderer 상태로 적용
        m_curSampler = mode;
    }

    std::pair<float , float> RenderSystem::ToScreen ( float wx , float wy ) const
    {
        int ox = 0 , oy = 0;
        if ( m_cam ) {
            auto off = m_cam->OffsetInt ( );
            ox = off.first; oy = off.second;
        }

        auto* d3d = dynamic_cast< D3D11Renderer* >( m_renderer );
        const float cx = d3d ? ( float ) d3d->Width ( ) * 0.5f : 0.f;
        const float cy = d3d ? ( float ) d3d->Height ( ) * 0.5f : 0.f;

        // 화면 중심 기준 스케일 → 오프셋(줌)
        const float z = ( m_cfg.zoom > 0.f ) ? m_cfg.zoom : 1.f;
        const float sx = ( wx - ox - cx ) * z + cx;
        const float sy = ( wy - oy - cy ) * z + cy;
        return { sx, sy };
    }

    void RenderSystem::DrawSprite ( const Tex2D& tex ,
                                  float wx , float wy , float w , float h ,
                                  const RECT* src ,
                                  uint32_t rgba ,
                                  float rotation ,
                                  float originX , float originY ,
                                  float /*z*/ , uint8_t /*sortBlend*/ )
    {
        if ( !m_batch ) return;

        // 월드→스크린
        auto [sx , sy] = ToScreen ( wx , wy );

        // 크기에 줌 반영
        const float z = ( m_cfg.zoom > 0.f ) ? m_cfg.zoom : 1.f;
        const float sw = w * z;
        const float sh = h * z;

        // 현재는 블렌드/샘플러 그룹 정렬을 SpriteBatch가 직접 지원하지 않는다고 가정
        // 필요 시 SpriteBatch에 확장된 DrawZ/키 지정 API를 연결하면 됨.
        m_batch->Draw ( tex , sx , sy , sw , sh , src , rgba , rotation , originX , originY );
    }

    int RenderSystem::BackbufferWidth ( ) const
    {
        if ( auto* d3d = dynamic_cast< const D3D11Renderer* >( m_renderer ) )
            return d3d->Width ( );
        return 0;
    }

    int RenderSystem::BackbufferHeight ( ) const
    {
        if ( auto* d3d = dynamic_cast< const D3D11Renderer* >( m_renderer ) )
            return d3d->Height ( );
        return 0;
    }

} // namespace engine
