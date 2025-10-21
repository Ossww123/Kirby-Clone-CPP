#pragma once
#include <windows.h>
#include "engine/Object.h"
#include "engine/Math.h"
#include "engine/PhysicsBody.h"
#include "engine/Anim.h"
#include "engine/Texture.h"

namespace game {

    class Player final : public engine::Object {
    public:
        // 초기 위치/크기를 바로 지정
        explicit Player ( RECT playBounds ,
                        float x = 100.f , float y = 100.f ,
                        float w = 56.f , float h = 56.f )
            : m_body ( playBounds )
        {
            m_body.SetSize ( w , h );
            m_body.SetPosition ( x , y );
        }

        // FSM이 입력/물리/충돌을 모두 처리하므로 여기서는 아무 것도 안 함
        void Update ( double /*fixedDt*/ , const engine::Input& /*input*/ ) override { /* FSM이 처리 */ }

        void Render ( HDC dc , int ox , int oy ) override {}

        // 외부에서 월드/크기 조정 시 PhysicsBody에 위임
        void SetBounds ( RECT b ) { m_body.SetBounds ( b ); }
        void SetSize ( float w , float h ) { m_body.SetSize ( w , h ); }
        void SetPosition ( float x , float y ) { m_body.SetPosition ( x , y ); }
        void SetVisualSize ( float w , float h ) { m_visW = w; m_visH = h; }

        engine::Vec2 Center ( ) const {
            int x , y , w , h; m_body.GetBounds ( x , y , w , h );
            return { x + w * 0.5f, y + h * 0.5f };
        }

        void GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }
        void GetVisualSize ( float& w , float& h ) const { w = m_visW; h = m_visH; }

        // 충돌/물리 접근
        engine::PhysicsBody&        Body ( ) { return m_body; }
        const engine::PhysicsBody&  Body ( ) const { return m_body; }

        // 애니메이션 / 텍스처
        engine::Animator*           Animator ( ) { return &m_anim; }
        const engine::Animator*     Animator ( ) const { return &m_anim; }

        void                        SetTexture ( const engine::Tex2D& t ) { m_tex = t; }
        const engine::Tex2D&        Texture ( ) const { return m_tex; }
        float                       m_visW{ 16.f } , m_visH{ 16.f };

    private:
        engine::PhysicsBody m_body;
        engine::Animator    m_anim{};
        engine::Tex2D       m_tex{};
    };

} // namespace game
