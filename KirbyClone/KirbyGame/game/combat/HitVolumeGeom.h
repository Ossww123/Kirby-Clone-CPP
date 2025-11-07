// game/combat/HitVolumeGeom.h
//
// Responsibility: Build world-space shapes for a HitVolume (box/circle/capsule).
// Non-Goals:      Physics queries; rendering; resource ownership.
// Call-Context:   Main thread.
//
#pragma once
#include "engine/util/Types.h"  // IntRect
#include "engine/util/Math.h"   // Vec2
#include "game/combat/HitVolume.h"

namespace game {

    struct HitVolumeGeom {
        static void BuildShape ( const HitVolume& hv ,
            /*out*/ engine::IntRect& outBox ,
            /*out*/ engine::Vec2& segA ,
            /*out*/ engine::Vec2& segB ,
            /*out*/ float& outRadius );
    };

} // namespace game
