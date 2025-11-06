//
// Responsibility: Implementation of D3D11Renderer.
// Notes: Heavy headers/pragma live here (not in the header).
//

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "engine/render/D3D11Renderer.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace engine {

    D3D11Renderer::~D3D11Renderer ( ) { Cleanup ( ); }

    bool D3D11Renderer::Initialize ( void* hwnd , int width , int height , bool vsync )
    {
        m_hWnd = hwnd;
        m_vsync = vsync;

        // 1) Device / Context
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        static const D3D_FEATURE_LEVEL levels[ ] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };
        D3D_FEATURE_LEVEL got{};
        ComPtr<ID3D11Device> dev;
        ComPtr<ID3D11DeviceContext> ctx;

        HRESULT hr = D3D11CreateDevice (
            nullptr , D3D_DRIVER_TYPE_HARDWARE , nullptr , flags ,
            levels , _countof ( levels ) , D3D11_SDK_VERSION ,
            dev.GetAddressOf ( ) , &got , ctx.GetAddressOf ( )
        );
#if defined(_DEBUG)
        if ( FAILED ( hr ) ) { // retry without debug layer
            flags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDevice (
                nullptr , D3D_DRIVER_TYPE_HARDWARE , nullptr , flags ,
                levels , _countof ( levels ) , D3D11_SDK_VERSION ,
                dev.GetAddressOf ( ) , &got , ctx.GetAddressOf ( )
            );
        }
#endif
        if ( FAILED ( hr ) ) return false;

        // Store raw
        m_device = dev.Detach ( );
        m_context = ctx.Detach ( );

        // 2) DXGI Factory2
        ComPtr<IDXGIDevice>  dxgiDevice;
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<IDXGIFactory2> factory2;
        static_cast< ID3D11Device* >( m_device )->QueryInterface ( IID_PPV_ARGS ( dxgiDevice.GetAddressOf ( ) ) );
        dxgiDevice->GetAdapter ( &adapter );
        adapter->GetParent ( __uuidof( IDXGIFactory2 ) , &factory2 );

        // 3) Swap chain (flip discard)
        DXGI_SWAP_CHAIN_DESC1 scd{};
        scd.Width = width;
        scd.Height = height;
        scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        scd.SampleDesc.Count = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.BufferCount = 2;
        scd.Scaling = DXGI_SCALING_STRETCH;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ComPtr<IDXGISwapChain1> sc1;
        hr = factory2->CreateSwapChainForHwnd (
            static_cast< ID3D11Device* >( m_device ) ,
            static_cast< HWND >( m_hWnd ) ,
            &scd , nullptr , nullptr , &sc1
        );
        if ( FAILED ( hr ) ) return false;

        sc1.CopyTo ( &m_swapChain );

        // Disable Alt+Enter default toggle
        factory2->MakeWindowAssociation ( static_cast< HWND >( m_hWnd ) , DXGI_MWA_NO_ALT_ENTER );

        // 4) RTV + viewport
        if ( !CreateBackbufferRTV ( ) ) return false;
        SetViewport ( width , height );

        m_width = width;
        m_height = height;
        return true;
    }

    void D3D11Renderer::Resize ( int width , int height )
    {
        if ( !m_swapChain ) return;

        if ( m_context )
            m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );

        if ( m_rtv ) { m_rtv->Release ( );        m_rtv = nullptr; }
        if ( m_backBuffer ) { m_backBuffer->Release ( ); m_backBuffer = nullptr; }

        // Request resize (CreateBackbufferRTV will fetch actual size)
        m_swapChain->ResizeBuffers ( 0 , width , height , DXGI_FORMAT_UNKNOWN , 0 );

        CreateBackbufferRTV ( ); // also updates viewport/width/height
    }

    void D3D11Renderer::BeginFrame ( Color clear )
    {
        const float c[ 4 ] = { clear.r, clear.g, clear.b, clear.a };
        if ( m_context && m_rtv ) {
            m_context->OMSetRenderTargets ( 1 , &m_rtv , nullptr );
            m_context->ClearRenderTargetView ( m_rtv , c );
        }
    }

    void D3D11Renderer::EndFrame ( )
    {
        if ( m_swapChain )
            m_swapChain->Present ( m_vsync ? 1 : 0 , 0 );
    }

    BackbufferSize D3D11Renderer::GetBackbufferSize ( ) const
    {
        return { m_width, m_height };
    }

    bool D3D11Renderer::GetD3D11Handles ( ID3D11Device** dev , ID3D11DeviceContext** ctx )
    {
        if ( dev ) *dev = m_device;
        if ( ctx ) *ctx = m_context;
        return ( m_device && m_context );
    }

    ID3D11Device* D3D11Renderer::Device ( )  const { return m_device; }
    ID3D11DeviceContext* D3D11Renderer::Context ( ) const { return m_context; }
    IDXGISwapChain* D3D11Renderer::SwapChain ( ) const { return m_swapChain; }

    bool D3D11Renderer::CreateBackbufferRTV ( )
    {
        if ( !m_swapChain || !m_device ) return false;

        ComPtr<ID3D11Texture2D> bb;
        if ( FAILED ( m_swapChain->GetBuffer ( 0 , __uuidof( ID3D11Texture2D ) , &bb ) ) ) return false;

        ID3D11RenderTargetView* rtv = nullptr;
        if ( FAILED ( m_device->CreateRenderTargetView ( bb.Get ( ) , nullptr , &rtv ) ) ) return false;

        // Query actual size
        D3D11_TEXTURE2D_DESC desc{};
        bb->GetDesc ( &desc );
        m_width = static_cast< int >( desc.Width );
        m_height = static_cast< int >( desc.Height );

        SetViewport ( m_width , m_height );

        // Hold references
        if ( m_backBuffer ) m_backBuffer->Release ( );
        m_backBuffer = bb.Get ( ); m_backBuffer->AddRef ( );

        if ( m_rtv ) m_rtv->Release ( );
        m_rtv = rtv;

        return true;
    }

    void D3D11Renderer::SetViewport ( int w , int h )
    {
        if ( !m_context ) return;

        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.f;
        vp.TopLeftY = 0.f;
        vp.Width = static_cast< float >( w );
        vp.Height = static_cast< float >( h );
        vp.MinDepth = 0.f;
        vp.MaxDepth = 1.f;

        m_context->RSSetViewports ( 1 , &vp );
    }

    void D3D11Renderer::Cleanup ( )
    {
        if ( m_context )
            m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );

        if ( m_rtv ) { m_rtv->Release ( );        m_rtv = nullptr; }
        if ( m_backBuffer ) { m_backBuffer->Release ( ); m_backBuffer = nullptr; }
        if ( m_swapChain ) { m_swapChain->Release ( );  m_swapChain = nullptr; }
        if ( m_context ) { m_context->Release ( );    m_context = nullptr; }
        if ( m_device ) { m_device->Release ( );     m_device = nullptr; }

        m_width = m_height = 0;
        m_hWnd = nullptr;
    }

} // namespace engine
