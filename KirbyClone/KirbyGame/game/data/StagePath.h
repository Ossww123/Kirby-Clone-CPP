#pragma once
//
// Responsibility: Build asset file paths from canonical stage IDs (e.g., "t1/hub").
// Non-Goals:      Filesystem existence checks, localization, runtime search paths.
// Call-Context:   Header-only; safe to include from game/frontend/core.
//

#include <string>
#include <string_view>

namespace game {

    // "t1/hub" -> "assets/stages/t1/hub/stage.json"
    inline std::string StageJsonPathFromId ( std::string_view id ) {
        if ( id.empty ( ) ) return "assets/stages/t1/hub/stage.json"; // safe default
        std::string path; path.reserve ( 32 + id.size ( ) );
        path += "assets/stages/";
        path.append ( id.data ( ) , id.size ( ) );
        path += "/stage.json";
        return path;
    }

} // namespace game
