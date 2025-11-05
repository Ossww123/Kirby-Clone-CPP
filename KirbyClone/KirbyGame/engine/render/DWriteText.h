#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <wrl/client.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dxgi.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include <string>

namespace engine {

    // D3D11 백버퍼(스왑체인) 위에 D2D/DirectWrite로 텍스트를 그리는 경량 HUD
    class DWriteTextHUD {
    public:
        ~DWriteTextHUD ( ) { CleanupTarget ( ); }

        bool Initialize ( IDXGISwapChain* swapChain , float dpi = 96.0f ) {
            m_swapChain = swapChain;

            // Factories
            if ( FAILED ( D2D1CreateFactory ( D2D1_FACTORY_TYPE_SINGLE_THREADED , m_d2dFactory.ReleaseAndGetAddressOf ( ) ) ) )
                return false;

            if ( FAILED ( DWriteCreateFactory ( DWRITE_FACTORY_TYPE_SHARED ,
                __uuidof( IDWriteFactory ) , reinterpret_cast< IUnknown** >( m_dwriteFactory.ReleaseAndGetAddressOf ( ) ) ) ) )
                return false;

            // 기본 폰트/브러시 준비 (RenderTarget이 생기면 브러시 생성)
            if ( FAILED ( m_dwriteFactory->CreateTextFormat (
                L"Consolas" , nullptr , DWRITE_FONT_WEIGHT_NORMAL , DWRITE_FONT_STYLE_NORMAL ,
                DWRITE_FONT_STRETCH_NORMAL , 16.0f , L"ko-kr" , &m_textFormat ) ) )
                return false;

            m_textFormat->SetTextAlignment ( DWRITE_TEXT_ALIGNMENT_LEADING );
            m_textFormat->SetParagraphAlignment ( DWRITE_PARAGRAPH_ALIGNMENT_NEAR );

            return RecreateTarget ( ); // 현재 백버퍼로 D2D RenderTarget 생성
        }

        // 리사이즈/백버퍼 교체 시 반드시 호출
        bool RecreateTarget ( ) {
            CleanupTarget ( );

            Microsoft::WRL::ComPtr<IDXGISurface> surface;
            if ( FAILED ( m_swapChain->GetBuffer ( 0 , __uuidof( IDXGISurface ) , &surface ) ) )
                return false;

            // D2D RenderTarget 생성 (BGRA, 알파 무시)
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties (
                D2D1_RENDER_TARGET_TYPE_DEFAULT ,
                D2D1::PixelFormat ( DXGI_FORMAT_B8G8R8A8_UNORM , D2D1_ALPHA_MODE_IGNORE ) ,
                96.0f , 96.0f );

            if ( FAILED ( m_d2dFactory->CreateDxgiSurfaceRenderTarget ( surface.Get ( ) , &props , &m_rt ) ) )
                return false;

            // 흰색 브러시
            if ( FAILED ( m_rt->CreateSolidColorBrush ( D2D1::ColorF ( 1 , 1 , 1 , 1 ) , &m_brWhite ) ) )
                return false;

            return true;
        }

        void Begin ( ) {
            if ( !m_rt ) return;
            m_rt->BeginDraw ( );
            m_rt->SetTransform ( D2D1::Matrix3x2F::Identity ( ) );
        }

        // 간단한 한 줄 텍스트
        void DrawTextLine ( const std::wstring& text , float x , float y , const D2D1_COLOR_F& color = D2D1::ColorF ( 1 , 1 , 1 , 1 ) ) {
            if ( !m_rt ) return;
            m_brWhite->SetColor ( color );
            // 충분히 큰 레이아웃 박스 (오른쪽으로 길게)
            D2D1_RECT_F rect = D2D1::RectF ( x , y , x + 4096.0f , y + 1024.0f );
            m_rt->DrawText ( text.c_str ( ) , ( UINT32 ) text.size ( ) , m_textFormat.Get ( ) , rect , m_brWhite.Get ( ) );
        }

        void End ( ) {
            if ( !m_rt ) return;
            m_rt->EndDraw ( ); // 실패 시(디바이스 로스트) RecreateTarget 필요
        }

    private:
        void CleanupTarget ( ) {
            m_brWhite.Reset ( );
            m_rt.Reset ( );
        }

    private:
        Microsoft::WRL::ComPtr<ID2D1Factory>       m_d2dFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory>     m_dwriteFactory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormat;

        Microsoft::WRL::ComPtr<ID2D1RenderTarget>  m_rt;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brWhite;

        IDXGISwapChain* m_swapChain = nullptr; // 소유 X
    };

} // namespace engine
