#pragma once
//
// Responsibility: Apply hub cover unlocks based on SaveData (erase cover tiles + rebuild colliders).
// Non-Goals:      File I/O beyond CSV read; rendering.
// Call-Context:   Main thread; invoked during hub stage load.
//
#include "protocol/SaveSchema.h"
#include "game/data/StageDesc.h"
#include "game/data/StageCSV.h"
#include "engine/world/WorldSystem.h"

namespace game {

    inline void ApplyHubCoverUnlocks ( const StageDesc& desc ,
                                       const protocol::SaveData* save ,
                                       engine::WorldSystem& world )
    {
        if ( !save ) return;
        if ( desc.unlocks.empty ( ) ) return;

        std::vector<UnlockCSV> rows;
        if ( !LoadUnlocksCSV ( desc.unlocks.c_str ( ) , rows ) ) return;

        for ( const auto& u : rows ) {
            if ( protocol::IsCleared ( *save , u.require ) ) {
                world.EraseCoverRectTiles ( u.tx , u.ty , u.w , u.h );
            }
        }
        world.RebuildColliders ( );
    }

} // namespace game
