#pragma once
//
// Responsibility: Debug-draw adapter for HitVolume (box/circle/capsule).
// Non-Goals:      Regular rendering, gameplay rules.
// Call-Context:   Main thread during debug pass.
//
namespace engine { class IDebugDraw; }
namespace game { class HitVolume; }

namespace game {
    struct HitVolumeDebugDraw {
        static void Draw ( const HitVolume& hv , engine::IDebugDraw* dbg , int ox , int oy );
    };
}
