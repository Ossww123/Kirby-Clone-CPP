//
// Responsibility: Small public wrappers to enter/exit overlay states from game flow.
// Non-Goals:      Implement state logic itself (handled in state classes).
// Call-Context:   Main thread.
//
#include "game/entities/player/PlayerFSM.h"

namespace game {

    void PlayerFSM::BeginDance ( ) {
        // convert to Dance ZState. Z_Dance take state logic.
        RequestOver ( std::make_unique<Z_Dance> ( ) , ZState::Dance );
    }

    void PlayerFSM::EndDance ( ) {
        // finish overlay
        RequestOver ( std::make_unique<Z_None> ( ) , ZState::None );
    }

    void PlayerFSM::BeginDoorEnter ( ) {
        RequestOver ( std::make_unique<Z_DoorEnter> ( ) , ZState::DoorEnter ); 
    }

    void PlayerFSM::EndDoorEnter ( ) {
        RequestOver ( std::make_unique<Z_None> ( ) , ZState::None );
    }

} // namespace game
