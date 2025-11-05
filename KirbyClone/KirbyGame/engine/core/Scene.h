#pragma once
//
// Responsibility: Simple object container; Update/Render dispatch; spawning.
// Non-Goals:      Spatial queries, ordering, lifetime policies beyond Clear().
// Call-Context:   Main thread; call Update/Render once per frame.
//
#include <windows.h>
#include <vector>
#include <memory>
#include <utility>

namespace engine {

    class Input;            // fwd
    class Object;           // fwd

    class Scene
    {
    public:
        void Update ( double fixedDt , const Input& input );
        void Render ( HDC dc , int ox , int oy );

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
