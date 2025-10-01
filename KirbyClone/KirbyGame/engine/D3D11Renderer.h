#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>
#include <wrl/client.h>
#include "engine/IRenderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace engine {

    class D3D11Renderer final : public IRenderer {
    public:
        ~D3D11Renderer ( ) override { Cleanup ( ); }

        bool Initialize ( void* hwnd , int width , int height , bool vsync ) override
        {
            m_hWnd = static_cast< HWND >( hwnd );
            m_vsync = vsync;

            // 1) D3D11 Device/Context (BGRA 지원 + 디버그 레이어(있을 때만))
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            static const D3D_FEATURE_LEVEL levels[ ] = {
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
            if ( FAILED ( hr ) ) {
                // 디버그 레이어 미설치 환경 대비 재시도
                flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                hr = D3D11CreateDevice (
                    nullptr , D3D_DRIVER_TYPE_HARDWARE , nullptr , flags ,
                    levels , _countof ( levels ) ,
                    D3D11_SDK_VERSION ,
                    m_device.GetAddressOf ( ) , &got , m_context.GetAddressOf ( )
                );
            }
#endif
            if ( FAILED ( hr ) ) return false;

            // 2) DXGI Factory2 얻기
            Microsoft::WRL::ComPtr<IDXGIDevice>  dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            Microsoft::WRL::ComPtr<IDXGIFactory2> factory2;
            m_device.As ( &dxgiDevice );
            dxgiDevice->GetAdapter ( &adapter );
            adapter->GetParent ( __uuidof( IDXGIFactory2 ) , &factory2 );

            // 3) Flip-Model 스왑체인 생성 (FLIP_DISCARD)
            DXGI_SWAP_CHAIN_DESC1 scd{};
            scd.Width = width;
            scd.Height = height;
            scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;          // D2D/DirectWrite 인터옵 고려
            scd.SampleDesc.Count = 1;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scd.BufferCount = 2;
            scd.Scaling = DXGI_SCALING_STRETCH;
            scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            scd.Flags = 0; // 필요 시 ALLOW_TEARING 플래그 추가 가능

            Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
            hr = factory2->CreateSwapChainForHwnd (
                m_device.Get ( ) , m_hWnd , &scd , nullptr , nullptr , &sc1
            );
            if ( FAILED ( hr ) ) return false;

            sc1.As ( &m_swapChain );

            // Alt+Enter 기본 전체화면 전환 비활성
            factory2->MakeWindowAssociation ( m_hWnd , DXGI_MWA_NO_ALT_ENTER );

            // 4) RTV/뷰포트 설정
            if ( !CreateBackbufferRTV ( ) ) return false;
            SetViewport ( width , height );

            m_width = width; m_height = height;
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

            m_width = width; m_height = height;
        }

        void BeginFrame ( Color clear ) override {
            const float c[ 4 ] = { clear.r, clear.g, clear.b, clear.a };
            m_context->OMSetRenderTargets ( 1 , m_rtv.GetAddressOf ( ) , nullptr );
            m_context->ClearRenderTargetView ( m_rtv.Get ( ) , c );
        }

        void EndFrame ( ) override {
            m_swapChain->Present ( m_vsync ? 1 : 0 , 0 );
        }

        ID3D11Device* Device ( )  const { return m_device.Get ( ); }
        ID3D11DeviceContext* Context ( ) const { return m_context.Get ( ); }
        IDXGISwapChain* SwapChain ( ) const { return m_swapChain.Get ( ); }
        int Width ( )  const { return m_width; }
        int Height ( ) const { return m_height; }

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

        int m_width = 0 , m_height = 0;

        Microsoft::WRL::ComPtr<ID3D11Device>        m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain>      m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>     m_backBuffer;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    };

} // namespace engine
