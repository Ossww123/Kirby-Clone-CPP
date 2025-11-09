#pragma once
//
// Responsibility: Lightweight text HUD over a D3D11 swap chain using D2D/DirectWrite.
// Non-Goals: Rich text layout, paragraph styling, offscreen caching.
// Call-Context: Main thread; caller must have COM initialized.
//

#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d2d1.h>     // ID2D1Factory, ID2D1RenderTarget, ID2D1SolidColorBrush
#include <dwrite.h>   // IDWriteFactory, IDWriteTextFormat
#include <wrl/client.h>
#include "engine/render/IRenderer.h" // engine::Color

struct IDXGISwapChain;

namespace engine {

    class DWriteTextHUD {
    public:
        ~DWriteTextHUD ( );

        // Initialize with an existing DXGI swap chain. dpi default = 96.
        bool Initialize ( IDXGISwapChain* swapChain , float dpi = 96.0f );

        // Must be called on resize / backbuffer recreation.
        bool RecreateTarget ( );

        void Begin ( );
        // Draw a single line; color is engine::Color {r,g,b,a} in [0..1].
        void DrawTextLine ( const std::wstring& text , float x , float y , Color color = { 1,1,1,1 } );
        void End ( );

    private:
        void CleanupTarget ( );

    private:
        Microsoft::WRL::ComPtr<ID2D1Factory>          m_d2dFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory>        m_dwriteFactory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>     m_textFormat;

        Microsoft::WRL::ComPtr<ID2D1RenderTarget>     m_rt;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>  m_brush;

        IDXGISwapChain* m_swapChain = nullptr; // not owning
        float           m_dpi = 96.0f;
    };

} // namespace engine
