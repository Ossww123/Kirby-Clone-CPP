//
// Responsibility: Debug draw impl (cpp-only renderer/Windows deps).
// Non-Goals:      Regular rendering.
// Call-Context:   Main thread.
//

#include "game/debugdraw/MonsterDebugDraw.h"
#include "game/entities/monsters/Monster.h"

#include "engine/render/D3D11DebugDraw.h"
#include "engine/platform/win32/ColorUtil.h"  // engine::win32::RGBA8

namespace game {

    void MonsterDebugDraw::Draw ( const Monster& m , engine::D3D11DebugDraw* dbg , int ox , int oy ) {
        if ( !dbg || !m.Alive ( ) ) return;

        int x , y , w , h; m.GetBounds ( x , y , w , h );
        dbg->WorldRect ( x , y , w , h , ox , oy , engine::win32::RGBA8 ( 240 , 120 , 60 ) );

        const int hp = m.HP ( );
        const int maxHp = m.MaxHP ( );
        if ( maxHp > 0 && hp < maxHp ) {
            const int len = static_cast< int >( ( static_cast< float >( hp ) / maxHp ) * w );
            dbg->WorldLine ( x , y - 2 , x + len , y - 2 , ox , oy , engine::win32::RGBA8 ( 255 , 60 , 60 ) );
        }
    }

} // namespace game
