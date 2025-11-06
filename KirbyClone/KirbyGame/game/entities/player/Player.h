#pragma once
//
// Responsibility: Player entity handle — physics body, animator, texture.
// Non-Goals:      Gameplay logic (handled by FSM).
// Call-Context:   Main thread.
// Notes:          Headers use IntRect/RGBA8 only; no Windows types.
//

#include "engine/core/Object.h"         // base (complete type needed)
#include "engine/physics/PhysicsBody.h"    // member by value
#include "engine/util/Anim.h"           // member by value
#include "engine/render/Texture.h"        // member by value

namespace engine {
    struct IntRect;    // bounds (header stays Windows-free)
    struct Vec2;       // Center() return
    class  Input;      // Update() param
}

namespace game {

    class Player final : public engine::Object {
    public:
        // Construct with world bounds and initial pose.
        explicit Player ( const engine::IntRect& playBounds ,
                        float x = 100.f , float y = 100.f ,
                        float w = 56.f , float h = 56.f );

        // FSM drives I/O/physics/collision; this is a thin hook.
        void Update ( double fixedDt , const engine::Input& input ) override;

        // Delegate to PhysicsBody
        void SetBounds ( const engine::IntRect& b );
        void SetSize ( float w , float h );
        void SetPosition ( float x , float y );

        // Visual size (sprite basis)
        void SetVisualSize ( float w , float h );

        // Queries
        engine::Vec2 Center ( ) const;
        void GetBounds ( int& x , int& y , int& w , int& h ) const;
        void GetVisualSize ( float& w , float& h ) const;

        // Accessors
        engine::PhysicsBody& Body ( );
        const engine::PhysicsBody& Body ( ) const;

        engine::Animator* Animator ( );
        const engine::Animator* Animator ( ) const;

        void                       SetTexture ( const engine::Tex2D& t );
        const engine::Tex2D& Texture ( ) const;

    private:
        engine::PhysicsBody m_body;
        engine::Animator    m_anim{};
        engine::Tex2D       m_tex{};
        float               m_visW{ 16.f } , m_visH{ 16.f };
    };

} // namespace game
