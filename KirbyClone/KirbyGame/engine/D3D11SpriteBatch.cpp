#include "engine/D3D11SpriteBatch.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace engine {

    static inline uint64_t packHi ( BlendMode b , SamplerMode s , int16_t z )
    {
        // [ blend:8 | sampler:8 | z:16 | pad:32 ] (상위 32비트 사용)
        uint64_t hi = 0;
        hi |= ( uint64_t ( uint8_t ( b ) ) & 0xFFu ) << 56;
        hi |= ( uint64_t ( uint8_t ( s ) ) & 0xFFu ) << 48;
        hi |= ( uint64_t ( uint16_t ( z ) ) & 0xFFFFu ) << 32;
        return hi;
    }

    D3D11SpriteBatch::~D3D11SpriteBatch ( )
    {
        // ComPtr 자동 Release
    }

    bool D3D11SpriteBatch::Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH )
    {
        m_dev = dev; m_ctx = ctx;
        if ( !CreatePipeline ( ) ) return false;
        CreateStates ( );

        // 초기 버퍼 용량
        m_vbCapacity = 4096; // vertices
        m_ibCapacity = 6144; // indices (6 per sprite)

        // VB
        D3D11_BUFFER_DESC vbd{};
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage = D3D11_USAGE_DYNAMIC;
        vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbd.ByteWidth = UINT ( m_vbCapacity * sizeof ( SpriteVertex ) );
        if ( FAILED ( m_dev->CreateBuffer ( &vbd , nullptr , &m_vb ) ) ) return false;

        // IB
        D3D11_BUFFER_DESC ibd{};
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_DYNAMIC;
        ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ibd.ByteWidth = UINT ( m_ibCapacity * sizeof ( uint16_t ) );
        if ( FAILED ( m_dev->CreateBuffer ( &ibd , nullptr , &m_ib ) ) ) return false;

        SetProjection ( screenW , screenH );
        return true;
    }

    void D3D11SpriteBatch::CreateStates ( )
    {
        // Blend: Alpha
        D3D11_BLEND_DESC bd{}; bd.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        bd.RenderTarget[ 0 ].BlendEnable = TRUE;
        bd.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        m_dev->CreateBlendState ( &bd , &m_blendAlpha );

        // Blend: Additive
        bd.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_ONE;
        m_dev->CreateBlendState ( &bd , &m_blendAdd );

        // Samplers
        D3D11_SAMPLER_DESC sd{}; sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        m_dev->CreateSamplerState ( &sd , &m_sampLinear );
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        m_dev->CreateSamplerState ( &sd , &m_sampPoint );

        // Rasterizer
        D3D11_RASTERIZER_DESC rs{}; rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE; rs.DepthClipEnable = TRUE;
        m_dev->CreateRasterizerState ( &rs , &m_rsCullNone );
    }

    bool D3D11SpriteBatch::CreatePipeline ( )
    {
        static const char* VS_SRC = R"(
cbuffer CBProj : register(b0) { row_major float4x4 uProj; }
struct VSIn { float2 pos:POSITION; float2 uv:TEXCOORD0; float4 col:COLOR; };
struct VSOut{ float4 pos:SV_Position; float2 uv:TEXCOORD0; float4 col:COLOR; };
VSOut main(VSIn i){ VSOut o; o.pos = mul(float4(i.pos,0,1), uProj); o.uv=i.uv; o.col=i.col; return o; })";

        static const char* PS_SRC = R"(
Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);
float4 main(float4 pos:SV_Position, float2 uv:TEXCOORD0, float4 col:COLOR) : SV_Target {
    return tex0.Sample(samp0, uv) * col;
})";
        UINT flags = D3D11SpriteBatch::D3DCompileFlagsRowMajor ( );

        Microsoft::WRL::ComPtr<ID3DBlob> vsb , psb , err;
        if ( FAILED ( D3DCompile ( VS_SRC , strlen ( VS_SRC ) , nullptr , nullptr , nullptr , "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
        if ( FAILED ( D3DCompile ( PS_SRC , strlen ( PS_SRC ) , nullptr , nullptr , nullptr , "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

        if ( FAILED ( m_dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &m_vs ) ) ) return false;
        if ( FAILED ( m_dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &m_ps ) ) ) return false;

        D3D11_INPUT_ELEMENT_DESC il[ ] = {
            { "POSITION",0, DXGI_FORMAT_R32G32_FLOAT,   0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,   0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",   0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        if ( FAILED ( m_dev->CreateInputLayout ( il , _countof ( il ) , vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , &m_layout ) ) )
            return false;

        // Projection constant buffer (16 floats)
        D3D11_BUFFER_DESC cb{};
        cb.Usage = D3D11_USAGE_DEFAULT;
        cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb.ByteWidth = 16 * sizeof ( float );
        if ( FAILED ( m_dev->CreateBuffer ( &cb , nullptr , &m_cbProj ) ) ) return false;

        return true;
    }

    void D3D11SpriteBatch::SetProjection ( int w , int h )
    {
        // 픽셀→NDC, y 뒤집기
        float m[ 16 ] = {
            2.0f / w,  0,         0,  0,
            0,       -2.0f / h,   0,  0,
            0,        0,          1,  0,
           -1,        1,          0,  1
        };
        m_ctx->UpdateSubresource ( m_cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
    }

    void D3D11SpriteBatch::Begin ( )
    {
        m_items.clear ( );
        m_vertices.clear ( );
        m_indices.clear ( );
        m_inBegin = true;

        // 상태 캐시 리셋
        m_boundTex = nullptr;
        m_boundBlend = static_cast< BlendMode >( 0xFF );
        m_boundSampler = static_cast< SamplerMode >( 0xFF );

        // ★ 파이프라인 고정 바인드
        m_ctx->IASetInputLayout ( m_layout.Get ( ) );
        m_ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        m_ctx->VSSetShader ( m_vs.Get ( ) , nullptr , 0 );
        ID3D11Buffer* cbs[ ] = { m_cbProj.Get ( ) };
        m_ctx->VSSetConstantBuffers ( 0 , 1 , cbs );

        m_ctx->PSSetShader ( m_ps.Get ( ) , nullptr , 0 );
        // 기본 샘플러/블렌드/래스터
        ID3D11SamplerState* samp = m_sampLinear.Get ( );
        m_ctx->PSSetSamplers ( 0 , 1 , &samp );
        float bf[ 4 ] = { 1,1,1,1 };
        m_ctx->OMSetBlendState ( m_blendAlpha.Get ( ) , bf , 0xFFFFFFFF );
        m_ctx->RSSetState ( m_rsCullNone.Get ( ) );
    }

    void D3D11SpriteBatch::End ( )
    {
        m_inBegin = false;
        if ( m_items.empty ( ) ) return;

        // 정렬: 상위키(블렌드/샘플러/z) → 하위키(SRV 주소)
        std::sort ( m_items.begin ( ) , m_items.end ( ) ,
                  [ ] ( const SpriteItem& a , const SpriteItem& b ) {
                          if ( a.sortKeyHi != b.sortKeyHi ) return a.sortKeyHi < b.sortKeyHi;
                          return a.sortKeyLo < b.sortKeyLo;
                  } );

        flushBatches ( );
    }

    void D3D11SpriteBatch::Draw ( const Tex2D& tex ,
                                float x , float y , float w , float h ,
                                const RECT* srcPixels ,
                                uint32_t tintRGBA ,
                                float rotation , float originX , float originY )
    {
        Draw ( tex , x , y , w , h , srcPixels , tintRGBA , rotation , originX , originY ,
             /*z*/0 , m_defaultBlend , m_defaultSampler );
    }

    void D3D11SpriteBatch::Draw ( const Tex2D& tex ,
                                float x , float y , float w , float h ,
                                const RECT* srcPixels ,
                                uint32_t tintRGBA ,
                                float rotation , float originX , float originY ,
                                int16_t zSort , BlendMode blend , SamplerMode sampler )
    {
        if ( !m_inBegin ) return;

        SpriteItem it{};
        it.tex = &tex;
        it.src = srcPixels ? *srcPixels : RECT{ 0,0,tex.width, tex.height };
        it.x = x; it.y = y; it.w = w; it.h = h;
        it.rotation = rotation; it.originX = originX; it.originY = originY;
        it.rgba = tintRGBA;

        it.sortKeyHi = packHi ( blend , sampler , zSort );
        auto* srv = tex.srv.Get ( );
        it.sortKeyLo = static_cast< uint64_t >( reinterpret_cast< uintptr_t >( srv ) );

        m_items.emplace_back ( it );
    }

    void D3D11SpriteBatch::ensureVB ( size_t vertices )
    {
        if ( vertices <= m_vbCapacity ) return;
        m_vbCapacity = std::max ( vertices , m_vbCapacity * 2 );
        D3D11_BUFFER_DESC vbd{};
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage = D3D11_USAGE_DYNAMIC;
        vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbd.ByteWidth = UINT ( m_vbCapacity * sizeof ( SpriteVertex ) );
        m_vb.Reset ( );
        m_dev->CreateBuffer ( &vbd , nullptr , &m_vb );
    }

    void D3D11SpriteBatch::ensureIB ( size_t indices )
    {
        if ( indices <= m_ibCapacity ) return;
        m_ibCapacity = std::max ( indices , m_ibCapacity * 2 );
        D3D11_BUFFER_DESC ibd{};
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_DYNAMIC;
        ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ibd.ByteWidth = UINT ( m_ibCapacity * sizeof ( uint16_t ) );
        m_ib.Reset ( );
        m_dev->CreateBuffer ( &ibd , nullptr , &m_ib );
    }

    // 회전/정점 생성
    static inline void pushQuad ( std::vector<SpriteVertex>& verts ,
                                std::vector<uint16_t>& idx ,
                                const Tex2D& tex ,
                                const RECT& src ,
                                float x , float y , float w , float h ,
                                float rot , float ox , float oy ,
                                uint32_t rgba )
    {
        const float cx = x + ox;
        const float cy = y + oy;
        const float lx = -ox , rx = w - ox;
        const float ty = -oy , by = h - oy;

        auto tf = [ & ] ( float px , float py , float& oxo , float& oyo ) {
            if ( rot == 0.f ) { oxo = cx + px; oyo = cy + py; }
            else {
                const float c = std::cos ( rot ) , s = std::sin ( rot );
                oxo = cx + ( px * c - py * s );
                oyo = cy + ( px * s + py * c );
            }
            };

        float x0 , y0 , x1 , y1 , x2 , y2 , x3 , y3;
        tf ( lx , ty , x0 , y0 );
        tf ( rx , ty , x1 , y1 );
        tf ( rx , by , x2 , y2 );
        tf ( lx , by , x3 , y3 );

        const float u0 = float ( src.left ) / tex.width;
        const float v0 = float ( src.top ) / tex.height;
        const float u1 = float ( src.right ) / tex.width;
        const float v1 = float ( src.bottom ) / tex.height;

        const uint16_t base = ( uint16_t ) verts.size ( );
        verts.push_back ( { x0,y0,u0,v0,rgba } );
        verts.push_back ( { x1,y1,u1,v0,rgba } );
        verts.push_back ( { x2,y2,u1,v1,rgba } );
        verts.push_back ( { x3,y3,u0,v1,rgba } );
        idx.push_back ( base + 0 ); idx.push_back ( base + 1 ); idx.push_back ( base + 2 );
        idx.push_back ( base + 0 ); idx.push_back ( base + 2 ); idx.push_back ( base + 3 );
    }

    void D3D11SpriteBatch::flushBatches ( )
    {
        // 그룹 키: (blend, sampler, tex) 변경 시 플러시
        BlendMode   curBlend = BlendMode::Alpha;
        SamplerMode curSamp = SamplerMode::Point;
        const Tex2D* curTex = nullptr;

        m_vertices.reserve ( m_items.size ( ) * 4 );
        m_indices.reserve ( m_items.size ( ) * 6 );

        auto flushGroup = [ & ] ( ) {
            if ( m_vertices.empty ( ) ) return;

            ensureVB ( m_vertices.size ( ) );
            ensureIB ( m_indices.size ( ) );

            // VB 업데이트
            D3D11_MAPPED_SUBRESOURCE map{};
            if ( SUCCEEDED ( m_ctx->Map ( m_vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map ) ) ) {
                std::memcpy ( map.pData , m_vertices.data ( ) , m_vertices.size ( ) * sizeof ( SpriteVertex ) );
                m_ctx->Unmap ( m_vb.Get ( ) , 0 );
            }

            // IB 업데이트
            if ( SUCCEEDED ( m_ctx->Map ( m_ib.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map ) ) ) {
                std::memcpy ( map.pData , m_indices.data ( ) , m_indices.size ( ) * sizeof ( uint16_t ) );
                m_ctx->Unmap ( m_ib.Get ( ) , 0 );
            }

            // 상태/텍스처 바인드
            applyBlend ( curBlend );
            applySampler ( curSamp );
            applyTexture ( curTex );

            // IA & Draw (Begin에서 VS/PS/IL/CB는 바인드됨)
            UINT stride = sizeof ( SpriteVertex ) , offset = 0;
            ID3D11Buffer* vb = m_vb.Get ( );
            m_ctx->IASetVertexBuffers ( 0 , 1 , &vb , &stride , &offset );
            m_ctx->IASetIndexBuffer ( m_ib.Get ( ) , DXGI_FORMAT_R16_UINT , 0 );
            m_ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
            m_ctx->DrawIndexed ( ( UINT ) m_indices.size ( ) , 0 , 0 );

            m_vertices.clear ( );
            m_indices.clear ( );
            };

        for ( size_t i = 0; i < m_items.size ( ); ++i ) {
            const auto& it = m_items[ i ];
            BlendMode   b = static_cast< BlendMode >( ( it.sortKeyHi >> 56 ) & 0xFF );
            SamplerMode s = static_cast< SamplerMode >( ( it.sortKeyHi >> 48 ) & 0xFF );
            const Tex2D* t = it.tex;

            const bool groupBreak = ( i == 0 ) ? false : ( b != curBlend || s != curSamp || t != curTex );
            if ( groupBreak ) flushGroup ( );

            curBlend = b; curSamp = s; curTex = t;

            // 누적
            pushQuad ( m_vertices , m_indices , *t , it.src ,
                     it.x , it.y , it.w , it.h , it.rotation , it.originX , it.originY , it.rgba );
        }
        flushGroup ( );

        // 캐시 리셋
        m_boundTex = nullptr;
        m_boundBlend = static_cast< BlendMode >( 0xFF );
        m_boundSampler = static_cast< SamplerMode >( 0xFF );
    }

    void D3D11SpriteBatch::applyBlend ( BlendMode m )
    {
        if ( m == m_boundBlend ) return;
        float bf[ 4 ] = { 1,1,1,1 };
        ID3D11BlendState* bs = ( m == BlendMode::Alpha ) ? m_blendAlpha.Get ( ) : m_blendAdd.Get ( );
        m_ctx->OMSetBlendState ( bs , bf , 0xFFFFFFFF );
        m_boundBlend = m;
    }

    void D3D11SpriteBatch::applySampler ( SamplerMode m )
    {
        if ( m == m_boundSampler ) return;
        ID3D11SamplerState* s = ( m == SamplerMode::Point ) ? m_sampPoint.Get ( ) : m_sampLinear.Get ( );
        m_ctx->PSSetSamplers ( 0 , 1 , &s );
        m_boundSampler = m;
    }

    void D3D11SpriteBatch::applyTexture ( const Tex2D* t )
    {
        if ( t == m_boundTex ) return;
        ID3D11ShaderResourceView* srv = t ? t->srv.Get ( ) : nullptr;
        m_ctx->PSSetShaderResources ( 0 , 1 , &srv );
        m_boundTex = t;
    }

    UINT D3D11SpriteBatch::D3DCompileFlagsRowMajor ( )
    {
        UINT f = 0;
#if defined(_DEBUG)
        f |= ( D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION );
#endif
        f |= ( D3DCOMPILE_ENABLE_STRICTNESS);
        f |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
        return f;
    }


} // namespace engine
