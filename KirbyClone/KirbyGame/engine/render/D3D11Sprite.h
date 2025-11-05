#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstdint>
#include <stdexcept>

#include "engine/Texture.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace engine {
    // --- Sprite 렌더러(사각형 1개 단위 드로우) ---
    class D3D11SpriteRenderer {
    public:
        bool Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH ) {
            m_dev = dev; m_ctx = ctx;
            if ( !CreatePipeline ( ) ) return false;
            CreateStates ( );
            CreateBuffers ( );
            SetProjection ( screenW , screenH );
            return true;
        }

        void OnResize ( int screenW , int screenH ) {
            SetProjection ( screenW , screenH );
        }

        // dst: 픽셀 좌표 (x,y,w,h), src: 텍스처 픽셀 사각형(nullptr이면 전체)
        void Draw ( const Tex2D& tex , float x , float y , float w , float h ,
                  const RECT* srcPixels = nullptr , float tint[ 4 ] = nullptr ) {
            if ( !tex.srv ) return;

            // 정점(픽셀 좌표)
            struct V { float x , y , u , v; };
            V v[ 4 ];

            float u0 = 0.f , v0 = 0.f , u1 = 1.f , v1 = 1.f;
            if ( srcPixels ) {
                u0 = srcPixels->left / float ( tex.width );
                v0 = srcPixels->top / float ( tex.height );
                u1 = srcPixels->right / float ( tex.width );
                v1 = srcPixels->bottom / float ( tex.height );
            }
            // (좌상, 우상, 좌하, 우하) - D3D의 스크린 Y는 위가 +? 아님. 우리는 투영행렬에서 뒤집음.
            v[ 0 ] = { x,     y,     u0, v0 };
            v[ 1 ] = { x + w,   y,     u1, v0 };
            v[ 2 ] = { x,     y + h,   u0, v1 };
            v[ 3 ] = { x + w,   y + h,   u1, v1 };

            // VB 업데이트
            D3D11_MAPPED_SUBRESOURCE map{};
            m_ctx->Map ( m_vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map );
            std::memcpy ( map.pData , v , sizeof ( v ) );
            m_ctx->Unmap ( m_vb.Get ( ) , 0 );

            // Tint
            float color[ 4 ] = { 1,1,1,1 };
            if ( tint ) { color[ 0 ] = tint[ 0 ]; color[ 1 ] = tint[ 1 ]; color[ 2 ] = tint[ 2 ]; color[ 3 ] = tint[ 3 ]; }
            m_ctx->UpdateSubresource ( m_cbTint.Get ( ) , 0 , nullptr , color , 0 , 0 );

            // 파이프라인 바인드
            UINT stride = sizeof ( V ) , offset = 0;
            m_ctx->IASetInputLayout ( m_layout.Get ( ) );
            m_ctx->IASetVertexBuffers ( 0 , 1 , m_vb.GetAddressOf ( ) , &stride , &offset );
            m_ctx->IASetIndexBuffer ( m_ib.Get ( ) , DXGI_FORMAT_R16_UINT , 0 );
            m_ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

            m_ctx->VSSetShader ( m_vs.Get ( ) , nullptr , 0 );
            m_ctx->VSSetConstantBuffers ( 0 , 1 , m_cbProj.GetAddressOf ( ) );

            m_ctx->PSSetShader ( m_ps.Get ( ) , nullptr , 0 );
            m_ctx->PSSetShaderResources ( 0 , 1 , tex.srv.GetAddressOf ( ) );
            m_ctx->PSSetSamplers ( 0 , 1 , m_samp.GetAddressOf ( ) );
            m_ctx->PSSetConstantBuffers ( 0 , 1 , m_cbTint.GetAddressOf ( ) );

            float blendFactor[ 4 ]{};
            m_ctx->OMSetBlendState ( m_blendAlpha.Get ( ) , blendFactor , 0xFFFFFFFF );
            m_ctx->RSSetState ( m_rsCullNone.Get ( ) );

            m_ctx->DrawIndexed ( 6 , 0 , 0 );
        }

    private:
        void SetProjection ( int w , int h ) {
            // 픽셀 → NDC: x: 0..w → -1..+1, y: 0..h → +1..-1 (Y 뒤집음)
            float m[ 16 ] = {
                2.0f / w,  0,         0,  0,
                0,       -2.0f / h,   0,  0,
                0,        0,         1,  0,
               -1,        1,         0,  1
            };
            m_ctx->UpdateSubresource ( m_cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
        }

        bool CreatePipeline ( ) {
            // --- HLSL 소스 ---
            static const char* VS_SRC = R"(
            cbuffer CBProj : register(b0) { row_major float4x4 uProj; } // ★ row_major
            struct VSIn { float2 pos:POSITION; float2 uv:TEXCOORD0; };
            struct VSOut{ float4 pos:SV_Position; float2 uv:TEXCOORD0; };
            VSOut main(VSIn i){
                VSOut o;
                o.pos = mul(float4(i.pos,0,1), uProj);
                o.uv = i.uv;
                return o;
            })";

            static const char* PS_SRC = R"(
            cbuffer CBTint : register(b0) { float4 uTint; }
            Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);
            float4 main(float4 pos:SV_Position, float2 uv:TEXCOORD0) : SV_Target {
                float4 c = tex0.Sample(samp0, uv);
                return c * uTint;
            })";

            Microsoft::WRL::ComPtr<ID3DBlob> vsb , psb , err;

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            if ( FAILED ( D3DCompile ( VS_SRC , std::strlen ( VS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
            if ( FAILED ( D3DCompile ( PS_SRC , std::strlen ( PS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

            if ( FAILED ( m_dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &m_vs ) ) )
                return false;
            if ( FAILED ( m_dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &m_ps ) ) )
                return false;

            // 입력 레이아웃: float2 pos, float2 uv
            D3D11_INPUT_ELEMENT_DESC il[ ] = {
                { "POSITION",0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            if ( FAILED ( m_dev->CreateInputLayout ( il , 2 , vsb->GetBufferPointer ( ) ,
                vsb->GetBufferSize ( ) , &m_layout ) ) )
                return false;

            // 상수버퍼(프로젝션, tint)
            D3D11_BUFFER_DESC cb{};
            cb.Usage = D3D11_USAGE_DEFAULT; cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cb.ByteWidth = 16 * sizeof ( float ); // 4x4 matrix
            if ( FAILED ( m_dev->CreateBuffer ( &cb , nullptr , &m_cbProj ) ) ) return false;

            cb.ByteWidth = 4 * sizeof ( float );  // tint RGBA
            if ( FAILED ( m_dev->CreateBuffer ( &cb , nullptr , &m_cbTint ) ) ) return false;

            return true;
        }

        void CreateStates ( ) {
            // 샘플러(선형, 클램프)
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_dev->CreateSamplerState ( &sd , &m_samp );

            // 알파 블렌딩 (Non-premultiplied)
            D3D11_BLEND_DESC bd{};
            bd.RenderTarget[ 0 ].BlendEnable = TRUE;
            bd.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            m_dev->CreateBlendState ( &bd , &m_blendAlpha );

            // 래스터라이저 (백 컬링 끔)
            D3D11_RASTERIZER_DESC rs{};
            rs.FillMode = D3D11_FILL_SOLID;
            rs.CullMode = D3D11_CULL_NONE;
            rs.DepthClipEnable = TRUE;
            m_dev->CreateRasterizerState ( &rs , &m_rsCullNone );
        }

        void CreateBuffers ( ) {
            // 동적 VB (4정점)
            D3D11_BUFFER_DESC vb{};
            vb.Usage = D3D11_USAGE_DYNAMIC;
            vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vb.ByteWidth = sizeof ( float ) * 4 * 4;
            m_dev->CreateBuffer ( &vb , nullptr , &m_vb );

            // 고정 IB (0,1,2, 2,1,3)
            uint16_t idx[ 6 ] = { 0,1,2, 2,1,3 };
            D3D11_BUFFER_DESC ib{};
            ib.Usage = D3D11_USAGE_IMMUTABLE;
            ib.BindFlags = D3D11_BIND_INDEX_BUFFER;
            ib.ByteWidth = sizeof ( idx );
            D3D11_SUBRESOURCE_DATA srd{ idx, 0, 0 };
            m_dev->CreateBuffer ( &ib , &srd , &m_ib );
        }

    private:
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_ctx = nullptr;

        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_ps;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_cbProj;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_cbTint;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>  m_samp;
        Microsoft::WRL::ComPtr<ID3D11BlendState>    m_blendAlpha;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vb;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_ib;
    };

} // namespace engine
