#pragma once
//
// Responsibility: D3D11 renderer (swap chain + RTV/viewport).
// Non-Goals: Depth/MSAA, resource managers, window creation.
// Call-Context: Main thread.
//

#include "engine/render/IRenderer.h"

// Forward decls only (keep header light)
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;

namespace engine {

    class D3D11Renderer final : public IRenderer {
    public:
        ~D3D11Renderer ( ) override;

        // IRenderer
        bool Initialize ( void* hwnd , int width , int height , bool vsync ) override;
        void Resize ( int width , int height ) override;

        void BeginFrame ( Color clear ) override;
        void EndFrame ( ) override;

        BackbufferSize GetBackbufferSize ( ) const override;
        bool GetD3D11Handles ( ID3D11Device** dev , ID3D11DeviceContext** ctx ) override;

        // Convenience
        ID3D11Device* Device ( )  const;
        ID3D11DeviceContext* Context ( ) const;
        IDXGISwapChain* SwapChain ( ) const;
        int Width ( )  const { return m_width; }
        int Height ( ) const { return m_height; }

    private:
        bool CreateBackbufferRTV ( );      // build RTV from swap chain
        void SetViewport ( int w , int h );  // rasterizer viewport
        void Cleanup ( );                  // release all

    private:
        void* m_hWnd = nullptr;  // HWND (opaque)
        bool  m_vsync = false;

        int m_width = 0;
        int m_height = 0;

        // Raw COM pointers (released in Cleanup)
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;
        IDXGISwapChain* m_swapChain = nullptr;
        ID3D11Texture2D* m_backBuffer = nullptr;
        ID3D11RenderTargetView* m_rtv = nullptr;
    };

} // namespace engine
