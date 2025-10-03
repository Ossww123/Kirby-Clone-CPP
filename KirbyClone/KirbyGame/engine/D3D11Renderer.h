#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>      // Direct3D 11 API
#include <dxgi1_2.h>    // DXGI 1.2 (스왑체인 관리)
#include <dxgi.h>       // DXGI 기본 인터페이스
#include <wrl/client.h> // ComPtr (COM 객체 스마트 포인터)
#include "engine/IRenderer.h"

#pragma comment(lib, "d3d11.lib")  // Direct3D 11 라이브러리 링크
#pragma comment(lib, "dxgi.lib")   // DXGI 라이브러리 링크

namespace engine {

    class D3D11Renderer final : public IRenderer {
    public:
        ~D3D11Renderer ( ) override { Cleanup ( ); }

        bool Initialize ( void* hwnd , int width , int height , bool vsync ) override
        {
            m_hWnd = static_cast< HWND >( hwnd );  // 윈도우 핸들 저장
            m_vsync = vsync;  // 수직 동기화 설정 저장

            // 1) D3D11 Device/Context
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  // BGRA 포맷 지원 (Direct2D 호환)
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;  // 디버그 모드: 디버그 레이어 활성화
#endif
            // 지원할 기능 수준 목록 (높은 버전부터 시도)
            static const D3D_FEATURE_LEVEL levels[ ] = {
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
            };
            D3D_FEATURE_LEVEL got{};  // 실제로 생성된 기능 수준 저장

            // D3D11 디바이스와 디바이스 컨텍스트 생성
            HRESULT hr = D3D11CreateDevice (
                nullptr ,                        // 기본 어댑터 사용
                D3D_DRIVER_TYPE_HARDWARE ,       // 하드웨어 가속 사용
                nullptr ,                        // 소프트웨어 래스터라이저 미사용
                flags ,                          // 생성 플래그
                levels ,                         // 기능 수준 배열
                _countof ( levels ) ,            // 배열 크기
                D3D11_SDK_VERSION ,              // SDK 버전
                m_device.GetAddressOf ( ) ,      // 생성된 디바이스 저장
                &got ,                           // 실제 기능 수준 저장
                m_context.GetAddressOf ( )       // 생성된 컨텍스트 저장
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
            if ( FAILED ( hr ) ) return false;  // 디바이스 생성 실패

            // 2) DXGI Factory2 획득 (스왑체인 생성용)
            Microsoft::WRL::ComPtr<IDXGIDevice>  dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;     // 그래픽카드 정보
            Microsoft::WRL::ComPtr<IDXGIFactory2> factory2;

            m_device.As ( &dxgiDevice );                      // D3D11Device → DXGIDevice 변환
            dxgiDevice->GetAdapter ( &adapter );              // 어댑터(GPU) 정보 획득
            adapter->GetParent ( __uuidof( IDXGIFactory2 ) , &factory2 );  // Factory2 획득

            // 3) Flip-Model 스왑체인 생성 (FLIP_DISCARD 방식)
            DXGI_SWAP_CHAIN_DESC1 scd{};
            scd.Width = width;                                // 백버퍼 너비
            scd.Height = height;                              // 백버퍼 높이
            scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;          // BGRA 포맷 (D2D 호환)
            scd.SampleDesc.Count = 1;                         // 멀티샘플링 없음
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
            scd.BufferCount = 2;                              // 더블 버퍼링 (프론트 + 백)
            scd.Scaling = DXGI_SCALING_STRETCH;               // 윈도우 크기 변경 시 늘림
            scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // Flip 모델 (성능 우수)
            scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;           // 알파 채널 무시
            scd.Flags = 0;  // 플래그 (필요시 ALLOW_TEARING 추가 가능)

            Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
            // 윈도우 핸들과 연결된 스왑체인 생성
            hr = factory2->CreateSwapChainForHwnd (
                m_device.Get ( ) ,  // D3D11 디바이스
                m_hWnd ,            // 윈도우 핸들
                &scd ,              // 스왑체인 설정
                nullptr ,           // 전체화면 설정 (nullptr = 창모드)
                nullptr ,           // 출력 제한 없음
                &sc1                // 생성된 스왑체인1
            );
            if ( FAILED ( hr ) ) return false;

            sc1.As ( &m_swapChain );  // SwapChain1 → SwapChain 변환

            // Alt+Enter로 전체화면 전환 비활성화 (수동 제어)
            factory2->MakeWindowAssociation ( m_hWnd , DXGI_MWA_NO_ALT_ENTER );

            // 4) RTV(렌더 타겟 뷰) 생성 및 뷰포트 설정
            if ( !CreateBackbufferRTV ( ) ) return false;
            SetViewport ( width , height );

            m_width = width; m_height = height;
            return true;
        }

        // 윈도우 크기 변경 시 호출
        void Resize ( int width , int height ) override {
            if ( !m_swapChain ) return;

            // 렌더 타겟 바인딩 해제 (리사이즈 전 필수)
            m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );
            m_rtv.Reset ( );        // RTV 해제
            m_backBuffer.Reset ( ); // 백버퍼 해제

            // 스왑체인 버퍼 크기 조정 (0 = 기존 설정 유지)
            m_swapChain->ResizeBuffers ( 0 , width , height , DXGI_FORMAT_UNKNOWN , 0 );

            CreateBackbufferRTV ( );  // 새 크기로 RTV 재생성
            SetViewport ( width , height );  // 뷰포트 재설정

            m_width = width; m_height = height;
        }

        // 프레임 시작: 화면 클리어
        void BeginFrame ( Color clear ) override {
            const float c[ 4 ] = { clear.r, clear.g, clear.b, clear.a };  // RGBA 배열
            m_context->OMSetRenderTargets ( 1 , m_rtv.GetAddressOf ( ) , nullptr );  // RTV 바인딩
            m_context->ClearRenderTargetView ( m_rtv.Get ( ) , c );  // 지정된 색으로 클리어
        }

        // 프레임 종료: 화면에 표시 (Present)
        void EndFrame ( ) override {
            // Present(vsync 간격, 플래그)
            // vsync=true → 1 (모니터 주사율에 동기화)
            // vsync=false → 0 (즉시 표시)
            m_swapChain->Present ( m_vsync ? 1 : 0 , 0 );
        }

        // Getter 함수들
        ID3D11Device* Device ( )  const { return m_device.Get ( ); }
        ID3D11DeviceContext* Context ( ) const { return m_context.Get ( ); }
        IDXGISwapChain* SwapChain ( ) const { return m_swapChain.Get ( ); }
        int Width ( )  const { return m_width; }
        int Height ( ) const { return m_height; }

    private:
        // 백버퍼에서 렌더 타겟 뷰(RTV) 생성
        bool CreateBackbufferRTV ( ) {
            // 스왑체인의 0번 버퍼(백버퍼)를 Texture2D로 획득
            if ( FAILED ( m_swapChain->GetBuffer ( 0 , __uuidof( ID3D11Texture2D ) , &m_backBuffer ) ) )
                return false;

            // 백버퍼로부터 렌더 타겟 뷰 생성
            if ( FAILED ( m_device->CreateRenderTargetView ( m_backBuffer.Get ( ) , nullptr , &m_rtv ) ) )
                return false;
            return true;
        }

        // 뷰포트 설정 (렌더링 영역 지정)
        void SetViewport ( int w , int h ) {
            D3D11_VIEWPORT vp{};
            vp.TopLeftX = 0.f;                        // 좌상단 X
            vp.TopLeftY = 0.f;                        // 좌상단 Y
            vp.Width = static_cast< float >( w );     // 너비
            vp.Height = static_cast< float >( h );    // 높이
            vp.MinDepth = 0.f;                        // 깊이 최소값 (0 = 가까움)
            vp.MaxDepth = 1.f;                        // 깊이 최대값 (1 = 멀리)
            m_context->RSSetViewports ( 1 , &vp );    // 래스터라이저에 뷰포트 설정
        }

        // 리소스 정리
        void Cleanup ( ) {
            if ( m_context )
                m_context->OMSetRenderTargets ( 0 , nullptr , nullptr );  // RTV 바인딩 해제
            m_rtv.Reset ( );        // 렌더 타겟 뷰 해제
            m_backBuffer.Reset ( ); // 백버퍼 해제
            m_swapChain.Reset ( );  // 스왑체인 해제
            m_context.Reset ( );    // 디바이스 컨텍스트 해제
            m_device.Reset ( );     // 디바이스 해제
        }

    private:
        HWND m_hWnd = nullptr;  // 윈도우 핸들
        bool m_vsync = false;   // 수직 동기화 여부

        int m_width = 0 , m_height = 0;  // 백버퍼 크기

        // ComPtr: COM 객체 자동 참조 카운팅 스마트 포인터
        Microsoft::WRL::ComPtr<ID3D11Device>        m_device;      // D3D11 디바이스 (리소스 생성)
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;     // 디바이스 컨텍스트 (렌더링 명령)
        Microsoft::WRL::ComPtr<IDXGISwapChain>      m_swapChain;   // 스왑체인 (버퍼 교환)
        Microsoft::WRL::ComPtr<ID3D11Texture2D>     m_backBuffer;  // 백버퍼 텍스처
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;      // 렌더 타겟 뷰
    };

} // namespace engine
