#include "engine/physics/CollisionDebugDraw.h"
#include "engine/render/D3D11DebugDraw.h"

namespace engine::physics {

    void DebugDraw ( const CollisionSystem& sys ,
                   engine::D3D11DebugDraw& dbg ,
                   int ox , int oy ,
                   std::uint32_t solid ,
                   std::uint32_t oneWay )
    {
        for ( const IntRect& r : sys.Statics ( ) )
            dbg.WorldRect ( r.l , r.t , r.r - r.l , r.b - r.t , ox , oy , solid );

        for ( const IntRect& r : sys.OneWays ( ) )
            dbg.WorldRect ( r.l , r.t , r.r - r.l , r.b - r.t , ox , oy , oneWay );
    }

} // namespace engine::physics
