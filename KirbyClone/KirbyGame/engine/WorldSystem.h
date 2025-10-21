#pragma once
#include <windows.h>
#include <string>
#include "engine/TileSet.h"
#include "engine/TileMap.h"
#include "engine/Collision.h"
#include "engine/D3D11SpriteBatch.h"

namespace engine {

    class WorldSystem {
    public:
        bool Load ( ID3D11Device* dev ,
                  const std::wstring& tilesPng ,
                  const std::wstring& csv ,
                  int tileW , int tileH );

        // 개별 단계 로딩/갱신
        bool LoadTileset ( ID3D11Device* dev , const std::wstring& tilesPng , int tileW , int tileH );
        bool LoadMapCSV ( const std::wstring& csv );
        void RebuildColliders ( ); // SOLID 병합 + ONEWAY 개별 등록

        // 렌더(가시 타일만)
        void RenderVisible ( D3D11SpriteBatch& batch , int ox , int oy , int screenW , int screenH ) const;
        void RenderVisibleScaled ( D3D11SpriteBatch & batch ,
                                    int ox , int oy , int screenW , int screenH ,
                                    int scale ) const;

        // 월드 크기(px) → 카메라 SetWorldRect에 사용
        RECT WorldRectPx ( ) const; // {0,0, mapW*tileW, mapH*tileH}

        // 접근자
        const TileSet& Tiles ( ) const { return m_tiles; }
        const TileMap& Map ( )   const { return m_map; }
        physics::CollisionSystem& Collision ( ) { return m_collision; }
        const physics::CollisionSystem& Collision ( ) const { return m_collision; }

        int TileW ( ) const { return m_tiles.TileW ( ); }
        int TileH ( ) const { return m_tiles.TileH ( ); }
        int MapW ( )  const { return m_map.W ( ); }
        int MapH ( )  const { return m_map.H ( ); }

        // 타일 정의 헬퍼(원웨이/솔리드 등 코드에서 정의할 때 사용)
        void DefineTile ( int id , const TileDef& def ) { m_tiles.Define ( id , def ); }

    private:
        TileSet                     m_tiles{};
        TileMap                     m_map{};
        physics::CollisionSystem    m_collision{};
    };

} // namespace engine
