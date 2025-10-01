#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#pragma comment(lib, "d3dcompiler.lib")

namespace engine {

    class D3D11DebugDraw {
    public:
        bool Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH ) {
            m_dev = dev; m_ctx = ctx;
            if ( !CreatePipeline ( ) ) return false;
            CreateStates ( );
            EnsureVB ( 2048 ); // 초기 용량(버텍스 개수)
            SetProjection ( screenW , screenH );
            return true;
        }

        void OnResize ( int w , int h ) { SetProjection ( w , h ); }
        void BeginFrame ( ) { m_verts.clear ( ); }

        // 스크린 좌표
        void Line ( int x1 , int y1 , int x2 , int y2 , COLORREF c ) {
            const uint32_t rgba = ToRGBA8 ( c );
            pushV ( ( float ) x1 , ( float ) y1 , rgba );
            pushV ( ( float ) x2 , ( float ) y2 , rgba );
        }
        void Rect ( int x , int y , int w , int h , COLORREF c ) {
            Line ( x , y , x + w , y , c );
            Line ( x + w , y , x + w , y + h , c );
            Line ( x + w , y + h , x , y + h , c );
            Line ( x , y + h , x , y , c );
        }

        // 월드 → 스크린 (오프셋 적용)
        void WorldLine ( int x1 , int y1 , int x2 , int y2 , int ox , int oy , COLORREF c ) {
            Line ( x1 - ox , y1 - oy , x2 - ox , y2 - oy , c );
        }
        void WorldRect ( int wx , int wy , int w , int h , int ox , int oy , COLORREF c ) {
            Rect ( wx - ox , wy - oy , w , h , c );
        }

        void Flush ( ) {
            if ( m_verts.empty ( ) ) return;
            EnsureVB ( ( UINT ) m_verts.size ( ) );

            // VB 업데이트
            D3D11_MAPPED_SUBRESOURCE map{};
            m_ctx->Map ( m_vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map );
            std::memcpy ( map.pData , m_verts.data ( ) , m_verts.size ( ) * sizeof ( Vertex ) );
            m_ctx->Unmap ( m_vb.Get ( ) , 0 );

            UINT stride = sizeof ( Vertex ) , offset = 0;
            m_ctx->IASetInputLayout ( m_layout.Get ( ) );
            m_ctx->IASetVertexBuffers ( 0 , 1 , m_vb.GetAddressOf ( ) , &stride , &offset );
            m_ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

            m_ctx->VSSetShader ( m_vs.Get ( ) , nullptr , 0 );
            m_ctx->VSSetConstantBuffers ( 0 , 1 , m_cbProj.GetAddressOf ( ) );
            m_ctx->PSSetShader ( m_ps.Get ( ) , nullptr , 0 );

            float blendFactor[ 4 ]{};
            m_ctx->OMSetBlendState ( m_blendAlpha.Get ( ) , blendFactor , 0xFFFFFFFF );
            m_ctx->RSSetState ( m_rsCullNone.Get ( ) );

            m_ctx->Draw ( ( UINT ) m_verts.size ( ) , 0 );
        }

    private:
        struct Vertex { float x , y; uint32_t rgba; };

        static uint32_t ToRGBA8 ( COLORREF c ) {
            uint32_t r = GetRValue ( c );
            uint32_t g = GetGValue ( c );
            uint32_t b = GetBValue ( c );
            return ( 0xFFu << 24 ) | ( r << 16 ) | ( g << 8 ) | b; // A R G B
        }
        void pushV ( float x , float y , uint32_t rgba ) { m_verts.push_back ( { x,y,rgba } ); }

        void SetProjection ( int w , int h ) {
            // 픽셀 → NDC (Y 뒤집음)
            float m[ 16 ] = {
                2.0f / w,  0,         0,  0,
                0,       -2.0f / h,   0,  0,
                0,        0,         1,  0,
               -1,        1,         0,  1
            };
            m_ctx->UpdateSubresource ( m_cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
        }

        void EnsureVB ( UINT neededVerts ) {
            if ( neededVerts <= m_capacity ) return;
            m_capacity = 1;
            while ( m_capacity < neededVerts ) m_capacity <<= 1; // 2배씩 증가

            D3D11_BUFFER_DESC vb{};
            vb.Usage = D3D11_USAGE_DYNAMIC;
            vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vb.ByteWidth = m_capacity * sizeof ( Vertex );
            m_vb.Reset ( );
            m_dev->CreateBuffer ( &vb , nullptr , &m_vb );
        }

        bool CreatePipeline ( ) {
            // HLSL (행렬은 row_major로)
            static const char* VS_SRC = R"(
cbuffer CBProj : register(b0) { row_major float4x4 uProj; }
struct VSIn { float2 pos:POSITION; float4 col:COLOR; };
struct VSOut{ float4 pos:SV_Position; float4 col:COLOR; };
VSOut main(VSIn i){ VSOut o; o.pos = mul(float4(i.pos,0,1), uProj); o.col = i.col; return o; })";

            static const char* PS_SRC = R"(
float4 main(float4 pos:SV_Position, float4 col:COLOR) : SV_Target { return col; })";

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if defined(_DEBUG)
            flags |= ( D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION );
#endif

            Microsoft::WRL::ComPtr<ID3DBlob> vsb , psb , err;
            if ( FAILED ( D3DCompile ( VS_SRC , std::strlen ( VS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
            if ( FAILED ( D3DCompile ( PS_SRC , std::strlen ( PS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

            if ( FAILED ( m_dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &m_vs ) ) )
                return false;
            if ( FAILED ( m_dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &m_ps ) ) )
                return false;

            D3D11_INPUT_ELEMENT_DESC il[ ] = {
                { "POSITION",0, DXGI_FORMAT_R32G32_FLOAT,        0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",   0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            if ( FAILED ( m_dev->CreateInputLayout ( il , 2 , vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , &m_layout ) ) )
                return false;

            // 상수버퍼(프로젝션)
            D3D11_BUFFER_DESC cb{};
            cb.Usage = D3D11_USAGE_DEFAULT;
            cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cb.ByteWidth = 16 * sizeof ( float );
            if ( FAILED ( m_dev->CreateBuffer ( &cb , nullptr , &m_cbProj ) ) ) return false;

            return true;
        }

        void CreateStates ( ) {
            // 알파 블렌딩 (투명선도 가능)
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

            D3D11_RASTERIZER_DESC rs{};
            rs.FillMode = D3D11_FILL_SOLID;
            rs.CullMode = D3D11_CULL_NONE;
            rs.DepthClipEnable = TRUE;
            m_dev->CreateRasterizerState ( &rs , &m_rsCullNone );
        }

    private:
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_ctx = nullptr;

        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_ps;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_cbProj;
        Microsoft::WRL::ComPtr<ID3D11BlendState>    m_blendAlpha;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vb;

        std::vector<Vertex> m_verts;
        UINT m_capacity = 0;
    };

} // namespace engine
