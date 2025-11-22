#pragma once
//
// Responsibility: Axis-aligned box collisions (overlap/MTV) and resolution against
//                  static blocks + one-way platforms, with optional ground snap.
// Non-Goals:      Continuous collision for fast movers, slope/triangle meshes,
//                 dynamic-dynamic resolution, broadphase partitioning.
// Call-Context:   Main thread; integer world coords (pixels).
//

#include <cstdint>
#include <vector>
#include <algorithm>     // std::min

#include "engine/util/Types.h"  // IntRect
#include "engine/util/Math.h"   // Vec2
#include "engine/physics/AABB.h" // Overlap/ResolveMTV, Int2

namespace engine { class D3D11DebugDraw; }

namespace engine::physics {

    struct CollisionReport {
        bool hitX = false;
        bool hitY = false;
        bool grounded = false; // contacted below

        // --- triggers (no resolution) ---
        bool inWater = false; // overlapping any water volume
        bool onLadder = false; // overlapping any ladder volume
    };

    struct CollisionParams {
        bool  enableGroundSnap = true;
        int   groundSnapPx = 1; // snap to solid tops if within N px (when falling/non-negative vy)
        int   oneWaySnapPx = 1; // snap to one-way top if within N px
    };

    class CollisionSystem {
    public:
        void Clear ( );

        void AddStaticBox ( const IntRect& r );
        void AddStaticBox ( int x , int y , int w , int h );

        void AddOneWayBox ( const IntRect& r );
        void AddOneWayBox ( int x , int y , int w , int h );

        // --- Triggers: water / ladder ---
        void AddWaterBox ( const IntRect& r );
        void AddWaterBox ( int x , int y , int w , int h );

        void AddLadderBox ( const IntRect& r );
        void AddLadderBox ( int x , int y , int w , int h );

        // prevBottom: previous frame bottom (pixels). Use INT32_MIN if unknown.
        void MoveAndCollide ( IntRect& aabb ,
                            engine::Vec2& vel ,
                            CollisionReport* out = nullptr ,
                            bool ignoreOneWay = false ,
                            int prevBottom = INT32_MIN ,
                            const CollisionParams& params = {} ) const;

        const std::vector<IntRect>& Statics ( ) const { return m_static; }
        const std::vector<IntRect>& OneWays ( ) const { return m_oneway; }
        const std::vector<IntRect>& Water ( ) const { return m_water; }
        const std::vector<IntRect>& Ladders ( ) const { return m_ladder; }

    private:
        std::vector<IntRect> m_static;
        std::vector<IntRect> m_oneway;

        // Trigger volumes
        std::vector<IntRect> m_water;
        std::vector<IntRect> m_ladder;
    };


} // namespace engine::physics
