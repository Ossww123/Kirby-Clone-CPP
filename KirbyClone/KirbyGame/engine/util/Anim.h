#pragma once
//
// Responsibility: Tiny sprite animation (clips/frames, time step).
// Non-Goals:      Texture load, rendering, blend trees.
// Call-Context:   Main thread; Update() no allocations.
//

#include <string>
#include <unordered_map>
#include <vector>
#include "engine/util/Types.h"   // IntRect

namespace engine {

    struct AnimFrame {
        IntRect  src;      // pixel rect in atlas
        float duration; // seconds
    };

    struct AnimClip {
        std::vector<AnimFrame> frames;
        bool                   loop = true;
    };

    class Animator {
    public:
        void AddClip ( const std::string& name , AnimClip clip );
        bool Play ( const std::string& name , bool reset = true );
        void Update ( double dt );
        void Clear ( );

        bool HasClip ( const std::string& name ) const;
        bool RemoveClip ( const std::string& name );

        const IntRect& CurrentSrc ( ) const;
        const std::string& CurrentName ( ) const { return m_curName; }

        static AnimClip MakeRowClip ( int startX , int startY , int cellW , int cellH ,
                                      int count , float fps , bool loop = true );

    private:
        std::unordered_map<std::string , AnimClip> m_clips;
        const AnimClip* m_cur = nullptr;
        std::string                                m_curName;
        int                                        m_idx = 0;
        float                                      m_t = 0.f;
    };

} // namespace engine
