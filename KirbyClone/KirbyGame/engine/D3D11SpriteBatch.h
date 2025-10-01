#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>

#include "engine/Texture.h"

namespace engine {

    /* 내부용: 누적 커맨드 */
    struct SBCommand {
        ID3D11ShaderResourceView* srv;
        float x , y , w , h;
        float u0 , v0 , u1 , v1;
        uint32_t rgba;        // 0xAARRGGBB
        float rot;            // 라디안
        float ox , oy;         // 회전 기준(픽셀, 좌상 원점)
    };

    class D3D11SpriteBatch {
    public:
        bool Initialize ( ID3D11Device* dev , ID3D11DeviceContext* ctx , int screenW , int screenH ) {
            m_dev = dev; m_ctx = ctx;
            if ( !CreatePipeline ( ) ) return false;
            CreateStates ( );
            EnsureVB ( 1024 );                 // 초기 1024 정점(=256 스프라이트)
            EnsureIB ( 1536 );                 // 초기 1536 인덱스(=256*6)
            SetProjection ( screenW , screenH );
            return true;
        }

        void OnResize ( int w , int h ) { SetProjection ( w , h ); }

        void SetSortByTexture ( bool on ) { m_sortByTexture = on; }

        void Begin ( ) { m_cmds.clear ( ); }

        // tintRGBA: 0xAARRGGBB (기본 = 흰색 불투명)
        void Draw ( const Tex2D& tex , float x , float y , float w , float h ,
                  const RECT* srcPixels = nullptr ,
                  uint32_t tintRGBA = 0xFFFFFFFF ,
                  float rotation = 0.f , float originX = 0.f , float originY = 0.f );

        void End ( ) { Flush ( ); }

    private:
        struct V { float x , y , u , v; uint32_t rgba; };

        void Flush ( );
        void SetProjection ( int w , int h ) {
            // 픽셀 → NDC (Y 뒤집기)
            float m[ 16 ] = {
                2.0f / w,  0,         0,  0,
                0,       -2.0f / h,   0,  0,
                0,        0,         1,  0,
               -1,        1,         0,  1
            };
            m_ctx->UpdateSubresource ( m_cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
        }

        static uint32_t RGBAfToU32 ( float r , float g , float b , float a ) {
            auto to8 = [ & ] ( float v ) { v = v < 0 ? 0 : ( v > 1 ? 1 : v ); return ( uint32_t ) ( v * 255.0f + 0.5f ); };
            return ( to8 ( a ) << 24 ) | ( to8 ( r ) << 16 ) | ( to8 ( g ) << 8 ) | to8 ( b );
        }

        bool CreatePipeline ( ) {
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
            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCompileFlagsRowMajor ( );
            Microsoft::WRL::ComPtr<ID3DBlob> vsb , psb , err;
            if ( FAILED ( D3DCompile ( VS_SRC , strlen ( VS_SRC ) , nullptr , nullptr , nullptr , "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
            if ( FAILED ( D3DCompile ( PS_SRC , strlen ( PS_SRC ) , nullptr , nullptr , nullptr , "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

            if ( FAILED ( m_dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &m_vs ) ) ) return false;
            if ( FAILED ( m_dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &m_ps ) ) ) return false;

            D3D11_INPUT_ELEMENT_DESC il[ ] = {
                { "POSITION",0, DXGI_FORMAT_R32G32_FLOAT,        0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,        0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",   0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            if ( FAILED ( m_dev->CreateInputLayout ( il , _countof ( il ) , vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , &m_layout ) ) )
                return false;

            // 상수버퍼(프로젝션)
            D3D11_BUFFER_DESC cb{};
            cb.Usage = D3D11_USAGE_DEFAULT; cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cb.ByteWidth = 16 * sizeof ( float );
            if ( FAILED ( m_dev->CreateBuffer ( &cb , nullptr , &m_cbProj ) ) ) return false;

            return true;
        }

        void CreateStates ( ) {
            // 샘플러(선형/클램프)
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_dev->CreateSamplerState ( &sd , &m_samp );

            // 알파 블렌딩 (non-premultiplied)
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

            // 래스터라이저
            D3D11_RASTERIZER_DESC rs{};
            rs.FillMode = D3D11_FILL_SOLID;
            rs.CullMode = D3D11_CULL_NONE;
            rs.DepthClipEnable = TRUE;
            m_dev->CreateRasterizerState ( &rs , &m_rsCullNone );
        }

        void EnsureVB ( UINT neededVerts ) {
            if ( neededVerts <= m_vbCapacity ) return;
            m_vbCapacity = 1; while ( m_vbCapacity < neededVerts ) m_vbCapacity <<= 1;
            D3D11_BUFFER_DESC vb{};
            vb.Usage = D3D11_USAGE_DYNAMIC;
            vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vb.ByteWidth = m_vbCapacity * sizeof ( V );
            m_vb.Reset ( );
            m_dev->CreateBuffer ( &vb , nullptr , &m_vb );
        }

        void EnsureIB ( UINT neededIdx ) {
            // 필요한 스프라이트 개수 = 인덱스 / 6 (올림)
            UINT needSprites = ( neededIdx + 5 ) / 6;

            // 용량을 2배씩 키우되, '스프라이트 개수' 기준으로 관리
            UINT spriteCapacity = 1;
            while ( spriteCapacity < needSprites ) spriteCapacity <<= 1;

            UINT newIdxCapacity = spriteCapacity * 6;
            if ( newIdxCapacity <= m_ibCapacity ) return;

            m_ibCapacity = newIdxCapacity;

            // 인덱스 패턴 생성 (스프라이트당 6개, 정점은 4개)
            std::vector<uint16_t> idx ( m_ibCapacity );
            for ( UINT s = 0 , i = 0 , v = 0; s < spriteCapacity; ++s , i += 6 , v += 4 ) {
                idx[ i + 0 ] = ( uint16_t ) ( v + 0 );
                idx[ i + 1 ] = ( uint16_t ) ( v + 1 );
                idx[ i + 2 ] = ( uint16_t ) ( v + 2 );
                idx[ i + 3 ] = ( uint16_t ) ( v + 2 );
                idx[ i + 4 ] = ( uint16_t ) ( v + 1 );
                idx[ i + 5 ] = ( uint16_t ) ( v + 3 );
            }

            D3D11_BUFFER_DESC ib{};
            ib.Usage = D3D11_USAGE_IMMUTABLE;
            ib.BindFlags = D3D11_BIND_INDEX_BUFFER;
            ib.ByteWidth = ( UINT ) ( idx.size ( ) * sizeof ( uint16_t ) );
            D3D11_SUBRESOURCE_DATA srd{ idx.data ( ), 0, 0 };
            m_ib.Reset ( );
            m_dev->CreateBuffer ( &ib , &srd , &m_ib );
        }


        static UINT D3DCompileFlagsRowMajor ( ) {
            UINT f = 0;
#if defined(_DEBUG)
            f |= ( D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION );
#endif
            f |= ( D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR );
            return f;
        }

    private:
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_ctx = nullptr;

        Microsoft::WRL::ComPtr<ID3D11VertexShader>    m_vs;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>     m_ps;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>     m_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>          m_cbProj;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>    m_samp;
        Microsoft::WRL::ComPtr<ID3D11BlendState>      m_blendAlpha;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rsCullNone;
        Microsoft::WRL::ComPtr<ID3D11Buffer>          m_vb;
        Microsoft::WRL::ComPtr<ID3D11Buffer>          m_ib;
        UINT m_vbCapacity = 0 , m_ibCapacity = 0;

        std::vector<SBCommand> m_cmds;
        bool m_sortByTexture = true;
    };

    /* ----- inline 구현들 ----- */

    inline void D3D11SpriteBatch::Draw ( const Tex2D& tex , float x , float y , float w , float h ,
                                       const RECT* srcPixels , uint32_t tintRGBA ,
                                       float rotation , float originX , float originY )
    {
        SBCommand c{};
        c.srv = tex.srv.Get ( );
        c.x = x; c.y = y; c.w = w; c.h = h; c.rgba = tintRGBA; c.rot = rotation; c.ox = originX; c.oy = originY;

        if ( srcPixels ) {
            c.u0 = srcPixels->left / float ( tex.width );
            c.v0 = srcPixels->top / float ( tex.height );
            c.u1 = srcPixels->right / float ( tex.width );
            c.v1 = srcPixels->bottom / float ( tex.height );
        }
        else {
            c.u0 = 0; c.v0 = 0; c.u1 = 1; c.v1 = 1;
        }
        m_cmds.push_back ( c );
    }

    inline void D3D11SpriteBatch::Flush ( )
    {
        if ( m_cmds.empty ( ) ) return;

        // 정렬(텍스처별로 묶기)
        if ( m_sortByTexture ) {
            std::stable_sort ( m_cmds.begin ( ) , m_cmds.end ( ) ,
                [ ] ( const SBCommand& a , const SBCommand& b ) { return a.srv < b.srv; } );
        }

        // 파이프라인 바인드(고정)
        m_ctx->IASetInputLayout ( m_layout.Get ( ) );
        m_ctx->IASetIndexBuffer ( m_ib.Get ( ) , DXGI_FORMAT_R16_UINT , 0 );
        m_ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        m_ctx->VSSetShader ( m_vs.Get ( ) , nullptr , 0 );
        m_ctx->VSSetConstantBuffers ( 0 , 1 , m_cbProj.GetAddressOf ( ) );
        m_ctx->PSSetShader ( m_ps.Get ( ) , nullptr , 0 );
        m_ctx->PSSetSamplers ( 0 , 1 , m_samp.GetAddressOf ( ) );
        float blendFactor[ 4 ]{};
        m_ctx->OMSetBlendState ( m_blendAlpha.Get ( ) , blendFactor , 0xFFFFFFFF );
        m_ctx->RSSetState ( m_rsCullNone.Get ( ) );

        // 텍스처별로 끊어서 업로드/드로우
        size_t i = 0;
        while ( i < m_cmds.size ( ) ) {
            ID3D11ShaderResourceView* curSRV = m_cmds[ i ].srv;

            // 같은 텍스처 묶기
            size_t j = i + 1;
            while ( j < m_cmds.size ( ) && m_cmds[ j ].srv == curSRV ) ++j;

            const size_t count = ( j - i );               // 이 텍스처로 그릴 스프라이트 수
            const UINT vertsNeeded = ( UINT ) ( count * 4 );
            const UINT idxNeeded = ( UINT ) ( count * 6 );
            EnsureVB ( vertsNeeded );
            EnsureIB ( idxNeeded );

            // 정점 버퍼 채우기
            D3D11_MAPPED_SUBRESOURCE map{};
            m_ctx->Map ( m_vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map );
            V* vptr = static_cast< V* >( map.pData );

            for ( size_t k = i; k < j; ++k ) {
                const auto& c = m_cmds[ k ];

                // 회전 적용(원점: (ox,oy))
                const float cx = c.x + c.ox;
                const float cy = c.y + c.oy;
                const float sx = -c.ox , sy = -c.oy;
                const float ex = sx + c.w , ey = sy + c.h;

                auto rot = [ & ] ( float px , float py )->std::pair<float , float> {
                    if ( c.rot == 0.f ) return { cx + px, cy + py };
                    const float s = sinf ( c.rot ) , ccos = cosf ( c.rot );
                    float rx = px * ccos - py * s;
                    float ry = px * s + py * ccos;
                    return { cx + rx, cy + ry };
                    };

                auto [x0 , y0] = rot ( sx , sy );
                auto [x1 , y1] = rot ( ex , sy );
                auto [x2 , y2] = rot ( sx , ey );
                auto [x3 , y3] = rot ( ex , ey );

                vptr[ 0 ] = { x0,y0, c.u0,c.v0, c.rgba };
                vptr[ 1 ] = { x1,y1, c.u1,c.v0, c.rgba };
                vptr[ 2 ] = { x2,y2, c.u0,c.v1, c.rgba };
                vptr[ 3 ] = { x3,y3, c.u1,c.v1, c.rgba };
                vptr += 4;
            }
            m_ctx->Unmap ( m_vb.Get ( ) , 0 );

            UINT stride = sizeof ( V ) , offset = 0;
            m_ctx->IASetVertexBuffers ( 0 , 1 , m_vb.GetAddressOf ( ) , &stride , &offset );
            m_ctx->PSSetShaderResources ( 0 , 1 , &curSRV );
            m_ctx->DrawIndexed ( idxNeeded , 0 , 0 );

            i = j; // 다음 텍스처 그룹
        }
    }

} // namespace engine
