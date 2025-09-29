#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include "engine/IRenderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace engine {

    class D3D11Renderer final : public IRenderer {
    public:
        ~D3D11Renderer ( ) override { Cleanup ( ); }

        bool Initialize ( void* hwnd , int width , int height , bool vsync ) override {
            m_hWnd = static_cast< HWND >( hwnd );
            m_vsync = vsync;

            // 1) Device/Context
            UINT flags = 0;
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            static D3D_FEATURE_LEVEL levels[ ] = {
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
            };
            D3D_FEATURE_LEVEL got{};
            HRESULT hr = D3D11CreateDevice (
                nullptr , D3D_DRIVER_TYPE_HARDWARE , nullptr , flags ,
                levels , _countof ( levels ) ,
                D3D11_SDK_VERSION ,
                m_device.GetAddressOf ( ) , &got , m_context.GetAddressOf ( )
            );
#if defined(_DEBUG)
            if ( FAILED ( hr ) ) { // 디버그 레이어 미설치 환경 대비 재시도
                hr = D3D11CreateDevice (
                    nullptr , D3D_DRIVER_TYPE_HARDWARE , nullptr , 0 ,
                    levels , _countof ( levels ) ,
                    D3D11_SDK_VERSION ,
                    m_device.GetAddressOf ( ) , &got , m_context.GetAddressOf ( )
                );
            }
#endif
            if ( FAILED ( hr ) ) return false;

            // 2) SwapChain
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
            m_device.As ( &dxgiDevice );

            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            dxgiDevice->GetAdapter ( adapter.GetAddressOf ( ) );

            Microsoft::WRL::ComPtr<IDXGIFactory> factory;
            adapter->GetParent ( __uuidof( IDXGIFactory ) , &factory );

            DXGI_SWAP_CHAIN_DESC desc{};
            desc.BufferCount = 2;
            desc.BufferDesc.Width = width;
            desc.BufferDesc.Height = height;
            desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.OutputWindow = m_hWnd;
            desc.SampleDesc.Count = 1;
            desc.Windowed = TRUE;
            desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // (간단한 설정)
            if ( FAILED ( factory->CreateSwapChain ( m_device.Get ( ) , &desc , m_swapChain.ReleaseAndGetAddressOf ( ) ) ) )
                return false;

            // Alt+Enter 기본 동작 끄기(선택)
            factory->MakeWindowAssociation ( m_hWnd , DXGI_MWA_NO_ALT_ENTER );

            if ( !CreateBackbufferRTV ( ) ) return false;
            SetViewport ( width , height );
            return true;
        }

        void Resize ( int width , int height ) override {
            if ( !m_swapChain ) return;
            m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );
            m_rtv.Reset ( );
            m_backBuffer.Reset ( );
            m_swapChain->ResizeBuffers ( 0 , width , height , DXGI_FORMAT_UNKNOWN , 0 );
            CreateBackbufferRTV ( );
            SetViewport ( width , height );
        }

        void BeginFrame ( Color clear ) override {
            const float c[ 4 ] = { clear.r, clear.g, clear.b, clear.a };
            m_context->OMSetRenderTargets ( 1 , m_rtv.GetAddressOf ( ) , nullptr );
            m_context->ClearRenderTargetView ( m_rtv.Get ( ) , c );
        }

        void EndFrame ( ) override {
            m_swapChain->Present ( m_vsync ? 1 : 0 , 0 );
        }

    private:
        bool CreateBackbufferRTV ( ) {
            if ( FAILED ( m_swapChain->GetBuffer ( 0 , __uuidof( ID3D11Texture2D ) , &m_backBuffer ) ) )
                return false;
            if ( FAILED ( m_device->CreateRenderTargetView ( m_backBuffer.Get ( ) , nullptr , &m_rtv ) ) )
                return false;
            return true;
        }

        void SetViewport ( int w , int h ) {
            D3D11_VIEWPORT vp{};
            vp.TopLeftX = 0.f; vp.TopLeftY = 0.f;
            vp.Width = static_cast< float >( w );
            vp.Height = static_cast< float >( h );
            vp.MinDepth = 0.f; vp.MaxDepth = 1.f;
            m_context->RSSetViewports ( 1 , &vp );
        }

        void Cleanup ( ) {
            if ( m_context ) m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );
            m_rtv.Reset ( );
            m_backBuffer.Reset ( );
            m_swapChain.Reset ( );
            m_context.Reset ( );
            m_device.Reset ( );
        }

    private:
        HWND m_hWnd = nullptr;
        bool m_vsync = false;

        Microsoft::WRL::ComPtr<ID3D11Device>        m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain>      m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>     m_backBuffer;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace engine
