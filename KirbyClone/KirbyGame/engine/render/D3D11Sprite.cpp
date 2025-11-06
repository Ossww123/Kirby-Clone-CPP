//
// Responsibility: Implementation of D3D11SpriteRenderer.
// Notes: Heavy headers/pragma live here; header stays light.
// Requires: D3D11 device/context; non-PMA RGBA pipeline.
//

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "engine/render/D3D11Sprite.h"

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstring>   // memcpy

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace engine {

    struct D3D11SpriteRenderer::Impl {
        ID3D11Device* dev = nullptr;   // non-owning
        ID3D11DeviceContext* ctx = nullptr;   // non-owning

        ComPtr<ID3D11VertexShader>     vs;
        ComPtr<ID3D11PixelShader>      ps;
        ComPtr<ID3D11InputLayout>      layout;
        ComPtr<ID3D11Buffer>           cbProj;
        ComPtr<ID3D11Buffer>           cbTint;
        ComPtr<ID3D11SamplerState>     samp;
        ComPtr<ID3D11BlendState>       blendAlpha;
        ComPtr<ID3D11RasterizerState>  rsCullNone;
        ComPtr<ID3D11Buffer>           vb;
        ComPtr<ID3D11Buffer>           ib;

        struct Vtx { float x , y , u , v; };

        static void RGBA8_to_float4 ( std::uint32_t rgba , float out[ 4 ] ) {
            out[ 0 ] = ( ( rgba >> 16 ) & 0xFF ) / 255.0f; // R
            out[ 1 ] = ( ( rgba >> 8 ) & 0xFF ) / 255.0f; // G
            out[ 2 ] = ( rgba & 0xFF ) / 255.0f; // B
            out[ 3 ] = ( ( rgba >> 24 ) & 0xFF ) / 255.0f; // A
        }

        bool CreatePipeline ( ) {
            static const char* VS_SRC = R"(
cbuffer CBProj : register(b0) { row_major float4x4 uProj; }
struct VSIn { float2 pos:POSITION; float2 uv:TEXCOORD0; };
struct VSOut{ float4 pos:SV_Position; float2 uv:TEXCOORD0; };
VSOut main(VSIn i){ VSOut o; o.pos = mul(float4(i.pos,0,1), uProj); o.uv = i.uv; return o; })";

            static const char* PS_SRC = R"(
cbuffer CBTint : register(b0) { float4 uTint; }
Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);
float4 main(float4 pos:SV_Position, float2 uv:TEXCOORD0) : SV_Target {
    return tex0.Sample(samp0, uv) * uTint;
})";

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            ComPtr<ID3DBlob> vsb , psb , err;
            if ( FAILED ( D3DCompile ( VS_SRC , std::strlen ( VS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
            if ( FAILED ( D3DCompile ( PS_SRC , std::strlen ( PS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

            if ( FAILED ( dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &vs ) ) ) return false;
            if ( FAILED ( dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &ps ) ) ) return false;

            D3D11_INPUT_ELEMENT_DESC il[ ] = {
                { "POSITION",0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            if ( FAILED ( dev->CreateInputLayout ( il , 2 , vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , &layout ) ) ) return false;

            // Constant buffers
            D3D11_BUFFER_DESC cb{};
            cb.Usage = D3D11_USAGE_DEFAULT;
            cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cb.ByteWidth = 16 * sizeof ( float ); // 4x4
            if ( FAILED ( dev->CreateBuffer ( &cb , nullptr , &cbProj ) ) ) return false;

            cb.ByteWidth = 4 * sizeof ( float ); // tint rgba
            if ( FAILED ( dev->CreateBuffer ( &cb , nullptr , &cbTint ) ) ) return false;

            return true;
        }

        void CreateStates ( ) {
            // Sampler (linear clamp)
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            dev->CreateSamplerState ( &sd , &samp );

            // Alpha blend (non-PMA)
            D3D11_BLEND_DESC bd{};
            bd.RenderTarget[ 0 ].BlendEnable = TRUE;
            bd.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            dev->CreateBlendState ( &bd , &blendAlpha );

            // Rasterizer
            D3D11_RASTERIZER_DESC rs{};
            rs.FillMode = D3D11_FILL_SOLID;
            rs.CullMode = D3D11_CULL_NONE;
            rs.DepthClipEnable = TRUE;
            dev->CreateRasterizerState ( &rs , &rsCullNone );
        }

        void CreateBuffers ( ) {
            // Dynamic VB (4 vertices)
            D3D11_BUFFER_DESC vb{};
            vb.Usage = D3D11_USAGE_DYNAMIC;
            vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vb.ByteWidth = sizeof ( Vtx ) * 4;
            dev->CreateBuffer ( &vb , nullptr , &this->vb );

            // Immutable IB (0,1,2, 2,1,3)
            const std::uint16_t idx[ 6 ] = { 0,1,2, 2,1,3 };
            D3D11_BUFFER_DESC ib{};
            ib.Usage = D3D11_USAGE_IMMUTABLE;
            ib.BindFlags = D3D11_BIND_INDEX_BUFFER;
            ib.ByteWidth = sizeof ( idx );
            D3D11_SUBRESOURCE_DATA srd{ idx, 0, 0 };
            dev->CreateBuffer ( &ib , &srd , &this->ib );
        }
    };

    // ==== public API ====

    D3D11SpriteRenderer::D3D11SpriteRenderer ( ) = default;
    D3D11SpriteRenderer::~D3D11SpriteRenderer ( ) = default;
    D3D11SpriteRenderer::D3D11SpriteRenderer ( D3D11SpriteRenderer&& ) noexcept = default;
    D3D11SpriteRenderer& D3D11SpriteRenderer::operator=( D3D11SpriteRenderer&& ) noexcept = default;

    bool D3D11SpriteRenderer::Initialize ( void* d3dDevice , void* d3dContext , int screenW , int screenH )
    {
        m_impl = std::make_unique<Impl> ( );
        m_impl->dev = static_cast< ID3D11Device* >( d3dDevice );
        m_impl->ctx = static_cast< ID3D11DeviceContext* >( d3dContext );

        if ( !m_impl->CreatePipeline ( ) ) return false;
        m_impl->CreateStates ( );
        m_impl->CreateBuffers ( );
        SetProjection ( screenW , screenH );
        return true;
    }

    void D3D11SpriteRenderer::OnResize ( int screenW , int screenH ) {
        SetProjection ( screenW , screenH );
    }

    void D3D11SpriteRenderer::Draw ( const Tex2D& tex ,
                                   float x , float y , float w , float h ,
                                   const IntRect* srcPixels ,
                                   std::uint32_t tintRGBA )
    {
        if ( !tex.srv ) return;

        // Build vertices (pixel space); Y is flipped by projection
        Impl::Vtx v[ 4 ];

        float u0 = 0.f , v0 = 0.f , u1 = 1.f , v1 = 1.f;
        if ( srcPixels && tex.width > 0 && tex.height > 0 ) {
            u0 = srcPixels->l / float ( tex.width );
            v0 = srcPixels->t / float ( tex.height );
            u1 = srcPixels->r / float ( tex.width );
            v1 = srcPixels->b / float ( tex.height );
        }

        v[ 0 ] = { x,     y,     u0, v0 };
        v[ 1 ] = { x + w,   y,     u1, v0 };
        v[ 2 ] = { x,     y + h,   u0, v1 };
        v[ 3 ] = { x + w,   y + h,   u1, v1 };

        // Update VB
        D3D11_MAPPED_SUBRESOURCE map{};
        if ( SUCCEEDED ( m_impl->ctx->Map ( m_impl->vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map ) ) ) {
            std::memcpy ( map.pData , v , sizeof ( v ) );
            m_impl->ctx->Unmap ( m_impl->vb.Get ( ) , 0 );
        }

        // Update tint
        float tint[ 4 ];
        Impl::RGBA8_to_float4 ( tintRGBA , tint );
        m_impl->ctx->UpdateSubresource ( m_impl->cbTint.Get ( ) , 0 , nullptr , tint , 0 , 0 );

        // Bind pipeline
        UINT stride = sizeof ( Impl::Vtx ) , offset = 0;
        m_impl->ctx->IASetInputLayout ( m_impl->layout.Get ( ) );
        m_impl->ctx->IASetVertexBuffers ( 0 , 1 , m_impl->vb.GetAddressOf ( ) , &stride , &offset );
        m_impl->ctx->IASetIndexBuffer ( m_impl->ib.Get ( ) , DXGI_FORMAT_R16_UINT , 0 );
        m_impl->ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        m_impl->ctx->VSSetShader ( m_impl->vs.Get ( ) , nullptr , 0 );
        m_impl->ctx->VSSetConstantBuffers ( 0 , 1 , m_impl->cbProj.GetAddressOf ( ) );

        m_impl->ctx->PSSetShader ( m_impl->ps.Get ( ) , nullptr , 0 );
        m_impl->ctx->PSSetShaderResources ( 0 , 1 , tex.srv.GetAddressOf ( ) );
        m_impl->ctx->PSSetSamplers ( 0 , 1 , m_impl->samp.GetAddressOf ( ) );
        m_impl->ctx->PSSetConstantBuffers ( 0 , 1 , m_impl->cbTint.GetAddressOf ( ) );

        float blendFactor[ 4 ]{};
        m_impl->ctx->OMSetBlendState ( m_impl->blendAlpha.Get ( ) , blendFactor , 0xFFFFFFFF );
        m_impl->ctx->RSSetState ( m_impl->rsCullNone.Get ( ) );

        m_impl->ctx->DrawIndexed ( 6 , 0 , 0 );
    }

    void D3D11SpriteRenderer::SetProjection ( int w , int h )
    {
        // Pixel -> NDC; flip Y.
        const float m[ 16 ] = {
            2.0f / w,  0,          0, 0,
            0,        -2.0f / h,   0, 0,
            0,         0,          1, 0,
           -1.0f,      1.0f,       0, 1
        };
        m_impl->ctx->UpdateSubresource ( m_impl->cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
    }

} // namespace engine
