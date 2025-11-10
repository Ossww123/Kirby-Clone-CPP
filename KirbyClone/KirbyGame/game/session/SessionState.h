#pragma once
//
// Responsibility: Hold active save slot and a cached SaveData for the running session.
// Non-Goals:      Scene/Stage logic, fade/transition, UI rendering, multi-threading.
// Call-Context:   Owned by higher-level app/scene; synchronous use on main thread.
//
#include "protocol/SaveSchema.h"
#include "engine/save/SaveStorage.h"

namespace game {

    class SessionState {
    public:
        // Slot management
        void SetActiveSlot ( int slot ) noexcept;
        int  ActiveSlot ( ) const noexcept { return m_slot; }

        // Access current data
        const protocol::SaveData& Data ( ) const noexcept { return m_data; }
        protocol::SaveData& MutData ( ) noexcept { return m_data; }

        // Disk I/O (delegates to engine::SaveStorage)
        bool LoadFromDisk ( );
        bool SaveToDisk ( ) const;

        // Progress helpers
        int ProgressT1 ( ) const noexcept { return engine::SaveStorage::ProgressT1 ( m_data ); }

    private:
        int m_slot = 1;                    // 1..3
        protocol::SaveData m_data{};       // defaults from protocol
        engine::SaveStorage m_storage{};   // stateless helper
    };

} // namespace game
