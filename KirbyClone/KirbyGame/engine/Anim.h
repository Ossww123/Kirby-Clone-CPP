#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <windows.h>

namespace engine {

    struct AnimFrame {
        RECT  src;        // 텍스처 내 사각형(픽셀)
        float duration;   // 초
    };

    struct AnimClip {
        std::vector<AnimFrame> frames;
        bool loop = true;
    };

    class Animator {
    public:
        void AddClip ( const std::string& name , AnimClip clip ) { m_clips[ name ] = std::move ( clip ); }
        bool Play ( const std::string& name , bool reset = true ) {
            auto it = m_clips.find ( name );
            if ( it == m_clips.end ( ) ) return false;

            const AnimClip* next = &it->second;

            if ( next != m_cur ) {
                m_cur = next;
                m_curName = name;
                m_idx = 0; m_t = 0.f;
            }
            else if ( reset ) {
                m_idx = 0; m_t = 0.f;
            }
            return true;
        }
        void Update ( double dt ) {
            if ( !m_cur || m_cur->frames.empty ( ) ) return;
            m_t += static_cast< float >( dt );
            while ( m_t >= m_cur->frames[ m_idx ].duration ) {
                m_t -= m_cur->frames[ m_idx ].duration;
                ++m_idx;
                if ( m_idx >= static_cast< int >( m_cur->frames.size ( ) ) ) {
                    if ( m_cur->loop ) m_idx = 0;
                    else { m_idx = ( int ) m_cur->frames.size ( ) - 1; break; }
                }
            }
        }

        // 모든 클립 제거 + 상태 초기화
        void Clear ( ) {
            m_clips.clear ( );
            m_cur = nullptr;
            m_curName.clear ( );
            m_idx = 0;
            m_t = 0.f;
        }

        // 클립 존재 여부
        bool HasClip ( const std::string& name ) const {
            return m_clips.find ( name ) != m_clips.end ( );
        }

        // 특정 클립 제거 (현재 재생 중인 클립이면 상태도 리셋)
        bool RemoveClip ( const std::string& name ) {
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

        const RECT& CurrentSrc ( ) const {
            static RECT dummy{ 0,0,0,0 };
            return ( m_cur && !m_cur->frames.empty ( ) ) ? m_cur->frames[ m_idx ].src : dummy;
        }
        const std::string& CurrentName ( ) const { return m_curName; }

        // 유틸: 등간격 그리드에서 연속 프레임 만들기(가로로 count개)
        static AnimClip MakeRowClip ( int startX , int startY , int cellW , int cellH , int count , float fps , bool loop = true ) {
            AnimClip c; c.loop = loop;
            const float dur = ( fps > 0.f ) ? 1.0f / fps : 0.1f;
            for ( int i = 0; i < count; ++i ) {
                RECT r{ startX + i * cellW, startY, startX + ( i + 1 ) * cellW, startY + cellH };
                c.frames.push_back ( { r, dur } );
            }
            return c;
        }

    private:
        std::unordered_map<std::string , AnimClip> m_clips;
        const AnimClip* m_cur = nullptr;
        std::string m_curName;
        int   m_idx = 0;
        float m_t = 0.f;
    };

} // namespace engine
