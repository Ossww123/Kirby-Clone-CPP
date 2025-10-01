#pragma once
#include <windows.h>
#include <unordered_map>

#include "engine/Texture.h"

struct ID3D11Device;

namespace engine {

    struct TileDef {
        bool solid = false;
        bool oneway = false;     // v2에서 사용 예정
        RECT src{ 0,0,0,0 };        // atlas 내 픽셀 사각형
    };

    class TileSet {
    public:
        // 타일 아틀라스 로드 + 타일 크기 지정
        bool LoadAtlas ( ID3D11Device* dev , const wchar_t* path , int tileW , int tileH );

        void Define ( int id , const TileDef& def ) { m_defs[ id ] = def; }

        const TileDef* Get ( int id ) const {
            auto it = m_defs.find ( id );
            return ( it == m_defs.end ( ) ) ? nullptr : &it->second;
        }

        const Tex2D& Atlas ( ) const { return m_atlas; }
        int TileW ( ) const { return m_tileW; }
        int TileH ( ) const { return m_tileH; }

    private:
        Tex2D m_atlas{};
        int   m_tileW = 0;
        int   m_tileH = 0;
        std::unordered_map<int , TileDef> m_defs;
    };

} // namespace engine
