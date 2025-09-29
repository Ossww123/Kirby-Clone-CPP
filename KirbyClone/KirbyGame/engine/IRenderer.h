#pragma once

namespace engine {

    struct Color { float r , g , b , a; };

    class IRenderer {
    public:
        virtual ~IRenderer ( ) = default;

        virtual bool Initialize ( void* hwnd , int width , int height , bool vsync ) = 0;
        virtual void Resize ( int width , int height ) = 0;

        virtual void BeginFrame ( Color clear ) = 0;
        virtual void EndFrame ( ) = 0;
    };

} // namespace engine
