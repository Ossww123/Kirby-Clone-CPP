// engine/core/Scene.h
//
// Role: Simple object container that owns Objects and dispatches fixed-step Update.
// Note: No spatial queries, explicit ordering, or lifetime policy beyond Clear(). Call from main thread.
//

#pragma once

#include <vector>
#include <memory>
#include <utility>

namespace engine {

    class Input;
    class Object;

    class Scene
    {
    public:
        void Update ( double fixedDt , const Input& input );

        template <class T , class... Args>
        T* Spawn ( Args&&... args )
        {
            auto ptr = std::make_unique<T> ( std::forward<Args> ( args )... );
            T* raw = ptr.get ( );
            m_objects.emplace_back ( std::move ( ptr ) );
            return raw;
        }

        void Clear ( );

    private:
        std::vector<std::unique_ptr<Object>> m_objects;
    };

} // namespace engine
