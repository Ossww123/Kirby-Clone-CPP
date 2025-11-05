#include "engine/core/Scene.h"
#include "engine/core/Input.h"
#include "engine/core/Object.h"

namespace engine {

    void Scene::Update ( double fixedDt , const Input& input )
    {
        for ( auto& o : m_objects )
            o->Update ( fixedDt , input );
    }

    void Scene::Render ( HDC dc , int ox , int oy )
    {
        for ( auto& o : m_objects )
            o->Render ( dc , ox , oy );
    }

    void Scene::Clear ( )
    {
        m_objects.clear ( );
    }

} // namespace engine
