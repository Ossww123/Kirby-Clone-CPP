#pragma once
#include <atomic>
#include "engine/Input.h"

namespace engine {

    using EntityId = int32_t;

    inline EntityId GenEntityId ( ) {
        static std::atomic<EntityId> s_next{ 1 };
        return s_next++;
    }

    class Object {
    public:
        virtual ~Object ( ) = default;
        virtual void Update ( double fixedDt , const Input& input ) {}
        virtual void Render ( HDC dc , int ox , int oy ) {}

        // --- common Id ---
        EntityId Id ( ) const noexcept { return m_id; }

    protected:
        void SetId ( EntityId id ) noexcept { m_id = id; }

    private:
        EntityId m_id{ 0 };
    };
} // namespace engine
