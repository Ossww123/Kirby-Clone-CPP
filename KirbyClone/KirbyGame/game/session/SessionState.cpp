//
// Responsibility: Manage active slot and cached SaveData, delegating I/O to SaveStorage.
// Non-Goals:      Transition logic, hub blockers, gameplay flags decisions.
// Call-Context:   Main thread; trivial state holder.
//
#include "game/session/SessionState.h"

namespace game {

    void SessionState::SetActiveSlot ( int slot ) noexcept {
        if ( slot < 1 ) m_slot = 1;
        else if ( slot > 3 ) m_slot = 3;
        else               m_slot = slot;
    }

    bool SessionState::LoadFromDisk ( ) {
        protocol::SaveData tmp; // start from protocol defaults
        if ( !m_storage.Load ( m_slot , tmp ) ) {
            // Keep defaults if no file; treat as new game.
            m_data = protocol::SaveData{};
            return false;
        }
        m_data = std::move ( tmp );
        return true;
    }

    bool SessionState::SaveToDisk ( ) const {
        return m_storage.Save ( m_slot , m_data );
    }

} // namespace game
