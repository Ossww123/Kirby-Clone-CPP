//
// Responsibility: Player entity glue — wire PhysicsBody/Animator/Texture; no gameplay.
// Non-Goals:      State logic (FSM owns), rendering policy.
// Call-Context:   Main thread.
//

#include "game/entities/player/Player.h"

#include "engine/util/Types.h"   // IntRect
#include "engine/util/Math.h"    // Vec2
#include "engine/core/Input.h"   // Update signature requires complete type
#include "engine/core/Object.h"  // GenEntityId()

namespace game {

    Player::Player ( const engine::IntRect& playBounds ,
                   float x , float y ,
                   float w , float h )
        : m_body ( playBounds )
    {
        SetId ( engine::GenEntityId ( ) );
        m_body.SetSize ( w , h );
        m_body.SetPosition ( x , y );
    }

    void Player::Update ( double /*fixedDt*/ , const engine::Input& /*input*/ ) {
        // FSM handles input/physics/collision.
    }

    void Player::SetBounds ( const engine::IntRect& b ) { m_body.SetBounds ( b ); }
    void Player::SetSize ( float w , float h ) { m_body.SetSize ( w , h ); }
    void Player::SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
    void Player::SetVisualSize ( float w , float h ) { m_visW = w; m_visH = h; }

    engine::Vec2 Player::Center ( ) const {
        int x , y , w , h;
        m_body.GetBounds ( x , y , w , h );
        return { x + w * 0.5f, y + h * 0.5f };
    }

    void Player::GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }
    void Player::GetVisualSize ( float& w , float& h ) const { w = m_visW; h = m_visH; }

    engine::PhysicsBody& Player::Body ( ) { return m_body; }
    const engine::PhysicsBody& Player::Body ( ) const { return m_body; }

    engine::Animator* Player::Animator ( ) { return &m_anim; }
    const engine::Animator* Player::Animator ( ) const { return &m_anim; }

    void Player::SetTexture ( const engine::Tex2D& t ) { m_tex = t; }
    const engine::Tex2D& Player::Texture ( ) const { return m_tex; }

} // namespace game
