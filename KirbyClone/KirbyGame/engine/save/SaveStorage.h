#pragma once
//
// Responsibility: Persist/restore protocol::SaveData to/from disk (k=v text format).
// Non-Goals:      Business rules (progress, flags semantics), rendering, threading.
// Call-Context:   Main thread; small, synchronous file I/O using <filesystem>/<fstream>.
//
#include <string>
#include "protocol/SaveSchema.h"

namespace engine {

    class SaveStorage {
    public:
        // Path helper (creates "saves" dir when saving; not when just asking for path).
        static std::string SlotPath ( int slot );

        // Load/Save return false on error; on Load failure 'out' is left as-is.
        bool Load ( int slot , protocol::SaveData& out ) const noexcept;
        bool Save ( int slot , const protocol::SaveData& in ) const noexcept;

        // Convenience wrappers to the protocol helpers (kept here for ease-of-use).
        static inline void SetCleared ( protocol::SaveData& s , std::string_view stageId ) { protocol::SetCleared ( s , stageId ); }
        static inline bool IsCleared ( const protocol::SaveData& s , std::string_view stageId ) noexcept { return protocol::IsCleared ( s , stageId ); }
        static inline int  ProgressT1 ( const protocol::SaveData& s ) noexcept { return protocol::ProgressT1 ( s ); }
    };

} // namespace engine
