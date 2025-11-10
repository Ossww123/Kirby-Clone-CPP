#pragma once
//
// Responsibility: Define save-data contract & helpers shared across game/engine/editor/server.
// Non-Goals:      File I/O, logging, platform specifics, rendering/physics/gameplay types.
// Call-Context:   Header-only; include from any module (no external dependencies).
//

#include <string>
#include <string_view>
#include <unordered_map>
#include <array>
#include <cstdint>

namespace protocol {

    // ---- Versioning ----
    inline constexpr int kSaveVersion = 1;

    // ---- Canonical stage IDs for Theme 1 (t1) ----
    // NOTE: Use forward slashes '/' consistently in IDs.
    inline constexpr std::array<std::string_view , 5> kT1Stages = {
        "t1/s1/m1", "t1/s1/m2", "t1/s1/m3", "t1/s1/m4", "t1/s1/m5" // m5 = boss
    };

    // ---- Data model ----
    struct SaveData {
        int version = kSaveVersion;
        std::string lastHub = "t1/hub";
        std::string lastStage = "t1/hub";
        std::string lastSpawn = "default";
        std::unordered_map<std::string , bool> flags;  // e.g., flags["clear_t1/s1/m1"] = true
    };

    // ---- Flag helpers ----
    inline void SetCleared ( SaveData& s , std::string_view stageId ) {
        s.flags[ "clear_" + std::string ( stageId ) ] = true;
    }
    inline bool IsCleared ( const SaveData& s , std::string_view stageId ) noexcept {
        const auto it = s.flags.find ( "clear_" + std::string ( stageId ) );
        return it != s.flags.end ( ) && it->second;
    }

    // ---- Progress (Theme 1 only): 5 stages × 20% ----
    inline int ProgressT1 ( const SaveData& s ) noexcept {
        int cleared = 0;
        for ( auto id : kT1Stages ) if ( IsCleared ( s , id ) ) ++cleared;
        return cleared * 20;
    }

} // namespace protocol
