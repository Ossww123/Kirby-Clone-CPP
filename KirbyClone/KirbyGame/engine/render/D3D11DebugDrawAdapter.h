//
// Responsibility: Adapt D3D11DebugDraw to IDebugDraw.
// Non-Goals:      Own rendering logic; resource lifetime of the impl.
// Call-Context:   Main thread.
//
#pragma once
#include "engine/render/IDebugDraw.h"

namespace engine { class D3D11DebugDraw; }

namespace engine {

    class D3D11DebugDrawAdapter final : public IDebugDraw {
    public:
        explicit D3D11DebugDrawAdapter ( D3D11DebugDraw* impl ) : m_impl ( impl ) {}
        void WorldLine ( int x0 , int y0 , int x1 , int y1 , int ox , int oy , Rgba32 c ) override;
        void WorldRect ( int x , int y , int w , int h , int ox , int oy , Rgba32 c ) override;
    private:
        D3D11DebugDraw* m_impl{};
    };

} // namespace engine
