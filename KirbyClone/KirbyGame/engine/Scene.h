#pragma once
#include <vector>
#include <memory>
#include <utility>
#include <windows.h>
#include "engine/Object.h"

namespace engine {
    class Scene {
    public:
        void Update ( double fixedDt , const Input& input ) {
            for ( auto& o : m_objects ) o->Update ( fixedDt , input );
        }
        void Render ( HDC dc , int ox , int oy ) {
            for ( auto& o : m_objects ) o->Render ( dc , ox , oy );
        }
        template<class T , class ...Args>
        T* Spawn ( Args&&...args ) {
            auto ptr = std::make_unique<T> ( std::forward<Args> ( args )... );
            T* raw = ptr.get ( );
            m_objects.push_back ( std::move ( ptr ) );
            return raw;
        }
        void Clear ( ) { m_objects.clear ( ); }

    private:
        std::vector<std::unique_ptr<Object>> m_objects;
    };
} // namespace engine
