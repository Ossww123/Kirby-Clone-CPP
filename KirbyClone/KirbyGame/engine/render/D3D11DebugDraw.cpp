#include "engine/render/D3D11DebugDraw.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace engine {

    struct D3D11DebugDraw::Impl {
        // non-owning device/context
        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;

        // GPU resources
        ComPtr<ID3D11VertexShader>    vs;
        ComPtr<ID3D11PixelShader>     ps;
        ComPtr<ID3D11InputLayout>     layout;
        ComPtr<ID3D11Buffer>          cbProj;
        ComPtr<ID3D11BlendState>      blendAlpha;
        ComPtr<ID3D11RasterizerState> rsCullNone;
        ComPtr<ID3D11Buffer>          vb;

        struct Vertex { float x , y; uint32_t rgba; };
        std::vector<Vertex> verts;
        unsigned            capacity = 0;

        // --- helpers (GPU setup & CPU buffer) ---
        bool CreatePipeline ( ) {
            static const char* VS_SRC = R"(
cbuffer CBProj : register(b0) { row_major float4x4 uProj; }
struct VSIn { float2 pos:POSITION; uint col:COLOR; };
struct VSOut{ float4 pos:SV_Position; float4 col:COLOR; };
VSOut main(VSIn i){
    VSOut o;
    float4 c = float4(
        ((i.col >> 16) & 255) / 255.0,
        ((i.col >>  8) & 255) / 255.0,
        ( i.col        & 255) / 255.0,
        ((i.col >> 24) & 255) / 255.0
    );
    o.pos = mul(float4(i.pos,0,1), uProj);
    o.col = c;
    return o;
})";

            static const char* PS_SRC = R"(
float4 main(float4 pos:SV_Position, float4 col:COLOR) : SV_Target { return col; })";

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if defined(_DEBUG)
            flags |= ( D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION );
#endif

            ComPtr<ID3DBlob> vsb , psb , err;
            if ( FAILED ( D3DCompile ( VS_SRC , std::strlen ( VS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "vs_5_0" , flags , 0 , &vsb , &err ) ) ) return false;
            if ( FAILED ( D3DCompile ( PS_SRC , std::strlen ( PS_SRC ) , nullptr , nullptr , nullptr ,
                "main" , "ps_5_0" , flags , 0 , &psb , &err ) ) ) return false;

            if ( FAILED ( dev->CreateVertexShader ( vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , nullptr , &vs ) ) ) return false;
            if ( FAILED ( dev->CreatePixelShader ( psb->GetBufferPointer ( ) , psb->GetBufferSize ( ) , nullptr , &ps ) ) ) return false;

            D3D11_INPUT_ELEMENT_DESC il[ ] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32_UINT,  0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            if ( FAILED ( dev->CreateInputLayout ( il , 2 , vsb->GetBufferPointer ( ) , vsb->GetBufferSize ( ) , &layout ) ) ) return false;

            D3D11_BUFFER_DESC cb{};
            cb.Usage = D3D11_USAGE_DEFAULT;
            cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cb.ByteWidth = 16 * sizeof ( float ); // 4x4
            if ( FAILED ( dev->CreateBuffer ( &cb , nullptr , &cbProj ) ) ) return false;

            return true;
        }

        void CreateStates ( ) {
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

            D3D11_RASTERIZER_DESC rs{};
            rs.FillMode = D3D11_FILL_SOLID;
            rs.CullMode = D3D11_CULL_NONE;
            rs.DepthClipEnable = TRUE;
            dev->CreateRasterizerState ( &rs , &rsCullNone );
        }

        void EnsureVB ( unsigned neededVerts ) {
            if ( neededVerts <= capacity ) return;
            capacity = 1;
            while ( capacity < neededVerts ) capacity <<= 1;

            D3D11_BUFFER_DESC vb{};
            vb.Usage = D3D11_USAGE_DYNAMIC;
            vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vb.ByteWidth = capacity * sizeof ( Vertex );
            this->vb.Reset ( );
            dev->CreateBuffer ( &vb , nullptr , &this->vb );
        }

        void Push ( float x , float y , uint32_t rgba ) { verts.push_back ( { x,y,rgba } ); }
    };

    // ===== D3D11DebugDraw (API) =====

    D3D11DebugDraw::D3D11DebugDraw ( ) = default;
    D3D11DebugDraw::~D3D11DebugDraw ( ) = default;
    D3D11DebugDraw::D3D11DebugDraw ( D3D11DebugDraw&& ) noexcept = default;
    D3D11DebugDraw& D3D11DebugDraw::operator=( D3D11DebugDraw&& ) noexcept = default;

    bool D3D11DebugDraw::Initialize ( void* d3dDevice , void* d3dContext , int screenW , int screenH ) {
        m_impl = std::make_unique<Impl> ( );
        m_impl->dev = static_cast< ID3D11Device* >( d3dDevice );
        m_impl->ctx = static_cast< ID3D11DeviceContext* >( d3dContext );

        if ( !m_impl->CreatePipeline ( ) ) return false;
        m_impl->CreateStates ( );
        m_impl->EnsureVB ( 2048 );
        SetProjection ( screenW , screenH );
        return true;
    }

    void D3D11DebugDraw::OnResize ( int w , int h ) { SetProjection ( w , h ); }
    void D3D11DebugDraw::BeginFrame ( ) { m_impl->verts.clear ( ); }

    void D3D11DebugDraw::Line ( int x1 , int y1 , int x2 , int y2 , uint32_t rgba ) {
        m_impl->Push ( static_cast< float >( x1 ) , static_cast< float >( y1 ) , rgba );
        m_impl->Push ( static_cast< float >( x2 ) , static_cast< float >( y2 ) , rgba );
    }

    void D3D11DebugDraw::Rect ( int x , int y , int w , int h , uint32_t rgba ) {
        Line ( x , y , x + w , y , rgba );
        Line ( x + w , y , x + w , y + h , rgba );
        Line ( x + w , y + h , x , y + h , rgba );
        Line ( x , y + h , x , y , rgba );
    }

    void D3D11DebugDraw::Rect ( const IntRect& r , uint32_t rgba ) {
        Rect ( r.l , r.t , ( r.r - r.l ) , ( r.b - r.t ) , rgba );
    }

    void D3D11DebugDraw::WorldLine ( int x1 , int y1 , int x2 , int y2 , int ox , int oy , uint32_t rgba ) {
        Line ( x1 - ox , y1 - oy , x2 - ox , y2 - oy , rgba );
    }

    void D3D11DebugDraw::WorldRect ( int wx , int wy , int w , int h , int ox , int oy , uint32_t rgba ) {
        Rect ( wx - ox , wy - oy , w , h , rgba );
    }

    void D3D11DebugDraw::WorldRect ( const IntRect& r , int ox , int oy , uint32_t rgba ) {
        Rect ( r.l - ox , r.t - oy , ( r.r - r.l ) , ( r.b - r.t ) , rgba );
    }

    void D3D11DebugDraw::Flush ( ) {
        if ( m_impl->verts.empty ( ) ) return;

        m_impl->EnsureVB ( static_cast< unsigned >( m_impl->verts.size ( ) ) );

        D3D11_MAPPED_SUBRESOURCE map{};
        if ( SUCCEEDED ( m_impl->ctx->Map ( m_impl->vb.Get ( ) , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map ) ) ) {
            std::memcpy ( map.pData , m_impl->verts.data ( ) , m_impl->verts.size ( ) * sizeof ( Impl::Vertex ) );
            m_impl->ctx->Unmap ( m_impl->vb.Get ( ) , 0 );
        }

        UINT stride = sizeof ( Impl::Vertex ) , offset = 0;
        m_impl->ctx->IASetInputLayout ( m_impl->layout.Get ( ) );
        m_impl->ctx->IASetVertexBuffers ( 0 , 1 , m_impl->vb.GetAddressOf ( ) , &stride , &offset );
        m_impl->ctx->IASetPrimitiveTopology ( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

        m_impl->ctx->VSSetShader ( m_impl->vs.Get ( ) , nullptr , 0 );
        m_impl->ctx->VSSetConstantBuffers ( 0 , 1 , m_impl->cbProj.GetAddressOf ( ) );
        m_impl->ctx->PSSetShader ( m_impl->ps.Get ( ) , nullptr , 0 );

        float blendFactor[ 4 ]{};
        m_impl->ctx->OMSetBlendState ( m_impl->blendAlpha.Get ( ) , blendFactor , 0xFFFFFFFF );
        m_impl->ctx->RSSetState ( m_impl->rsCullNone.Get ( ) );

        m_impl->ctx->Draw ( static_cast< UINT >( m_impl->verts.size ( ) ) , 0 );
    }

    void D3D11DebugDraw::SetProjection ( int w , int h ) {
        // pixel -> NDC (flip Y)
        const float m[ 16 ] = {
            2.0f / w,  0,          0, 0,
            0,        -2.0f / h,   0, 0,
            0,         0,          1, 0,
           -1.0f,      1.0f,       0, 1
        };
        m_impl->ctx->UpdateSubresource ( m_impl->cbProj.Get ( ) , 0 , nullptr , m , 0 , 0 );
    }

} // namespace engine
