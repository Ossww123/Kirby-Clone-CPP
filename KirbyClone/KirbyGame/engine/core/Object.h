// engine/core/Object.h
//
// Role: Base game object with a common EntityId and virtual Update entry point.
// Note: No ownership/lifetime policy or physics/render details; main-thread Update only.
//

#pragma once

#include <cstdint>
#include <atomic>

namespace engine {

    class Input; // fwd

    using EntityId = int32_t;

    inline EntityId GenEntityId ( )
    {
        static std::atomic<EntityId> s_next{ 1 };
        return s_next++;
    }

    class Object
    {
    public:
        virtual ~Object ( ) = default;

        // overridable hooks (no-op default)
        virtual void Update ( double fixedDt , const Input& input ) { ( void ) fixedDt; ( void ) input; }

        // id
        EntityId Id ( ) const noexcept { return m_id; }

    protected:
        void SetId ( EntityId id ) noexcept { m_id = id; }

    private:
        EntityId m_id{ 0 };
    };

} // namespace engine
