#include "engine/util/Anim.h"

namespace engine {

    void Animator::AddClip ( const std::string& name , AnimClip clip )
    {
        m_clips[ name ] = std::move ( clip );
    }

    bool Animator::Play ( const std::string& name , bool reset )
    {
        auto it = m_clips.find ( name );
        if ( it == m_clips.end ( ) ) return false;

        const AnimClip* next = &it->second;

        if ( next != m_cur ) {
            m_cur = next;
            m_curName = name;
            m_idx = 0;
            m_t = 0.f;
        }
        else if ( reset ) {
            m_idx = 0;
            m_t = 0.f;
        }
        return true;
    }

    void Animator::Update ( double dt )
    {
        if ( !m_cur || m_cur->frames.empty ( ) ) return;

        m_t += static_cast< float >( dt );
        while ( m_t >= m_cur->frames[ m_idx ].duration ) {
            m_t -= m_cur->frames[ m_idx ].duration;
            ++m_idx;

            if ( m_idx >= static_cast< int >( m_cur->frames.size ( ) ) ) {
                if ( m_cur->loop ) m_idx = 0;
                else { m_idx = static_cast< int >( m_cur->frames.size ( ) ) - 1; break; }
            }
        }
    }

    void Animator::Clear ( )
    {
        m_clips.clear ( );
        m_cur = nullptr;
        m_curName.clear ( );
        m_idx = 0;
        m_t = 0.f;
    }

    bool Animator::HasClip ( const std::string& name ) const
    {
        return m_clips.find ( name ) != m_clips.end ( );
    }

    bool Animator::RemoveClip ( const std::string& name )
    {
        auto it = m_clips.find ( name );
        if ( it == m_clips.end ( ) ) return false;
        if ( m_cur == &it->second ) {
            m_cur = nullptr;
            m_curName.clear ( );
            m_idx = 0;
            m_t = 0.f;
        }
        m_clips.erase ( it );
        return true;
    }

    const RECT& Animator::CurrentSrc ( ) const
    {
        static RECT kEmpty{ 0,0,0,0 };
        return ( m_cur && !m_cur->frames.empty ( ) ) ? m_cur->frames[ m_idx ].src : kEmpty;
    }

    AnimClip Animator::MakeRowClip ( int startX , int startY , int cellW , int cellH ,
                                     int count , float fps , bool loop )
    {
        AnimClip c; c.loop = loop;
        const float dur = ( fps > 0.f ) ? 1.0f / fps : 0.1f;
        c.frames.reserve ( static_cast< size_t >( count ) );
        for ( int i = 0; i < count; ++i ) {
            RECT r{ startX + i * cellW , startY ,
                    startX + ( i + 1 ) * cellW , startY + cellH };
            c.frames.push_back ( { r , dur } );
        }
        return c;
    }

} // namespace engine
