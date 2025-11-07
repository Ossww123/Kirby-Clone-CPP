//
// Responsibility: Debug draw impl for Projectile using engine-agnostic IDebugDraw.
// Non-Goals:      Rendering policy or animation.
// Call-Context:   Main thread.
//

#include "game/debugdraw/ProjectileDebugDraw.h"
#include "game/projectile/Projectile.h"
#include "engine/render/IDebugDraw.h"

namespace {
    constexpr engine::Rgba32 RGBA8 ( unsigned r , unsigned g , unsigned b , unsigned a = 255 ) {
        return ( engine::Rgba32 ( a ) << 24 ) | ( engine::Rgba32 ( r ) << 16 )
            | ( engine::Rgba32 ( g ) << 8 ) | engine::Rgba32 ( b );
    }
}

namespace game {

    void ProjectileDebugDraw::Draw ( const Projectile& p , engine::IDebugDraw* dbg , int ox , int oy ) {
        if ( !dbg || !p.Alive ( ) ) return;
        int x , y , w , h; p.GetBounds ( x , y , w , h );
        dbg->WorldRect ( x , y , w , h , ox , oy , RGBA8 ( 255 , 230 , 0 ) );
    }

} // namespace game
