#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace engine {

    struct Color { float r , g , b , a; };
    struct BackbufferSize { int w , h; };

    class IRenderer {
    public:
        virtual ~IRenderer ( ) = default;

        virtual bool Initialize ( void* hwnd , int width , int height , bool vsync ) = 0;
        virtual void Resize ( int width , int height ) = 0;

        virtual void BeginFrame ( Color clear ) = 0;
        virtual void EndFrame ( ) = 0;

        virtual BackbufferSize GetBackbufferSize ( ) const = 0;
        virtual bool GetD3D11Handles ( ID3D11Device** dev , ID3D11DeviceContext** ctx ) { return false; }
    };

} // namespace engine
