#pragma once
//
// Responsibility: Backend-agnostic renderer interface.
// Non-Goals: Backend details, window creation, resource lifetime.
// Call-Context: Main thread.
//

// Forward decls (no heavy headers here)
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace engine {

    struct Color { float r , g , b , a; };
    struct BackbufferSize { int w , h; };

    class IRenderer {
    public:
        virtual ~IRenderer ( ) = default;

        // Init / window events
        virtual bool Initialize ( void* hwnd , int width , int height , bool vsync ) = 0;
        virtual void Resize ( int width , int height ) = 0;

        // Frame
        virtual void BeginFrame ( Color clear ) = 0;
        virtual void EndFrame ( ) = 0;

        // Queries
        virtual BackbufferSize GetBackbufferSize ( ) const = 0;

        // expose D3D11 handles when available
        // Default returns false (not provided by a backend).
        virtual bool GetD3D11Handles ( ID3D11Device** dev , ID3D11DeviceContext** ctx ) { return false; }
    };

} // namespace engine
