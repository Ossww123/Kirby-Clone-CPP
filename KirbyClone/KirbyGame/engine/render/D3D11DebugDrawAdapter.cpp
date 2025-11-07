#include "engine/render/D3D11DebugDrawAdapter.h"
#include "engine/render/D3D11DebugDraw.h"

namespace engine {

    void D3D11DebugDrawAdapter::WorldLine ( int x0 , int y0 , int x1 , int y1 , int ox , int oy , Rgba32 c ) {
        m_impl->WorldLine ( x0 , y0 , x1 , y1 , ox , oy , c );
    }
    void D3D11DebugDrawAdapter::WorldRect ( int x , int y , int w , int h , int ox , int oy , Rgba32 c ) {
        m_impl->WorldRect ( x , y , w , h , ox , oy , c );
    }

} // namespace engine
