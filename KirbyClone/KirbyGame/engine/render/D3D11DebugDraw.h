#pragma once
//
// Responsibility: 2D debug overlay (lines/rects) on D3D11.
// Non-Goals: 3D, text, thick lines, depth.
// Call-Context: Main thread with a valid D3D11 immediate context.
//

#include <cstdint>
#include <memory>
#include "engine/util/Types.h" // IntRect

namespace engine {

    class D3D11DebugDraw {
    public:
        D3D11DebugDraw ( );
        ~D3D11DebugDraw ( );

        D3D11DebugDraw ( const D3D11DebugDraw& ) = delete;
        D3D11DebugDraw& operator=( const D3D11DebugDraw& ) = delete;
        D3D11DebugDraw ( D3D11DebugDraw&& ) noexcept;
        D3D11DebugDraw& operator=( D3D11DebugDraw&& ) noexcept;

        // d3dDevice/d3dContext are ID3D11Device*/ID3D11DeviceContext*
        bool Initialize ( void* d3dDevice , void* d3dContext , int screenW , int screenH );

        void OnResize ( int w , int h );   // update projection
        void BeginFrame ( );             // clear CPU-side buffer
        void Flush ( );                  // upload & draw (line list)

        // Screen-space (rgba = 0xAARRGGBB)
        void Line ( int x1 , int y1 , int x2 , int y2 , uint32_t rgba );
        void Rect ( int x , int y , int w , int h , uint32_t rgba );
        void Rect ( const IntRect& r , uint32_t rgba );

        // World-space with camera offset (ox,oy)
        void WorldLine ( int x1 , int y1 , int x2 , int y2 , int ox , int oy , uint32_t rgba );
        void WorldRect ( int wx , int wy , int w , int h , int ox , int oy , uint32_t rgba );
        void WorldRect ( const IntRect& r , int ox , int oy , uint32_t rgba );

    private:
        void SetProjection ( int w , int h );

        struct Impl;                           // hidden D3D state
        std::unique_ptr<Impl> m_impl;
    };

} // namespace engine
