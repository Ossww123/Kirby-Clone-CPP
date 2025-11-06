//
// Responsibility: Implementation of DWriteTextHUD (D2D/DWrite over DXGI swap chain).
// Notes: Caller must CoInitializeEx; render target is recreated on resize.
// 

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "engine/render/DWriteText.h"

#include <Windows.h>
#include <dxgi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

namespace engine {

    DWriteTextHUD::~DWriteTextHUD ( ) {
        CleanupTarget ( );
    }

    bool DWriteTextHUD::Initialize ( IDXGISwapChain* swapChain , float dpi )
    {
        m_swapChain = swapChain;
        m_dpi = dpi;

        // Factories
        if ( FAILED ( D2D1CreateFactory ( D2D1_FACTORY_TYPE_SINGLE_THREADED ,
            m_d2dFactory.ReleaseAndGetAddressOf ( ) ) ) )
            return false;

        if ( FAILED ( DWriteCreateFactory ( DWRITE_FACTORY_TYPE_SHARED ,
            __uuidof( IDWriteFactory ) ,
            reinterpret_cast< IUnknown** >( m_dwriteFactory.ReleaseAndGetAddressOf ( ) ) ) ) )
            return false;

        // Default text format
        if ( FAILED ( m_dwriteFactory->CreateTextFormat (
            L"Consolas" , nullptr ,
            DWRITE_FONT_WEIGHT_NORMAL , DWRITE_FONT_STYLE_NORMAL , DWRITE_FONT_STRETCH_NORMAL ,
            16.0f , L"ko-kr" , &m_textFormat ) ) )
            return false;

        m_textFormat->SetTextAlignment ( DWRITE_TEXT_ALIGNMENT_LEADING );
        m_textFormat->SetParagraphAlignment ( DWRITE_PARAGRAPH_ALIGNMENT_NEAR );

        return RecreateTarget ( );
    }

    bool DWriteTextHUD::RecreateTarget ( )
    {
        CleanupTarget ( );
        if ( !m_swapChain ) return false;

        // Acquire DXGI surface from swap chain
        ComPtr<IDXGISurface> surface;
        if ( FAILED ( m_swapChain->GetBuffer ( 0 , __uuidof( IDXGISurface ) , &surface ) ) )
            return false;

        // Create D2D render target (BGRA, ignore alpha)
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties (
            D2D1_RENDER_TARGET_TYPE_DEFAULT ,
            D2D1::PixelFormat ( DXGI_FORMAT_B8G8R8A8_UNORM , D2D1_ALPHA_MODE_IGNORE ) ,
            m_dpi , m_dpi );

        if ( FAILED ( m_d2dFactory->CreateDxgiSurfaceRenderTarget ( surface.Get ( ) , &props , &m_rt ) ) )
            return false;

        // Default white brush
        if ( FAILED ( m_rt->CreateSolidColorBrush ( D2D1::ColorF ( 1 , 1 , 1 , 1 ) , &m_brush ) ) )
            return false;

        return true;
    }

    void DWriteTextHUD::Begin ( )
    {
        if ( !m_rt ) return;
        m_rt->BeginDraw ( );
        m_rt->SetTransform ( D2D1::Matrix3x2F::Identity ( ) );
    }

    void DWriteTextHUD::DrawTextLine ( const std::wstring& text , float x , float y , Color color )
    {
        if ( !m_rt || !m_brush ) return;

        m_brush->SetColor ( D2D1::ColorF ( color.r , color.g , color.b , color.a ) );

        // Large rightward layout box
        D2D1_RECT_F rect = D2D1::RectF ( x , y , x + 4096.0f , y + 1024.0f );
        m_rt->DrawText ( text.c_str ( ) ,
                       static_cast< UINT32 >( text.size ( ) ) ,
                       m_textFormat.Get ( ) ,
                       rect ,
                       m_brush.Get ( ) );
    }

    void DWriteTextHUD::End ( )
    {
        if ( !m_rt ) return;
        // If EndDraw fails (device lost), caller should call RecreateTarget()
        m_rt->EndDraw ( );
    }

    void DWriteTextHUD::CleanupTarget ( )
    {
        m_brush.Reset ( );
        m_rt.Reset ( );
    }

} // namespace engine
