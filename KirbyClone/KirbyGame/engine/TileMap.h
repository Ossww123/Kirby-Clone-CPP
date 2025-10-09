#pragma once
#include <vector>
#include <string>

#include "engine/TileSet.h"
#include "engine/Collision.h"   // physics::CollisionSystem

namespace engine {

    class D3D11SpriteBatch;

    class TileMap {
    public:
        // CSV: 각 줄이 한 행, 쉼표 구분. 빈칸/음수는 빈 타일로 간주
        bool LoadCSV ( const wchar_t* path );

        // SOLID 타일을 큰 직사각형으로 병합해 충돌 시스템에 등록
        void BuildSolidColliders ( physics::CollisionSystem& cs , const TileSet& tiles ) const;

        // 가시 영역만 렌더
        void Render ( D3D11SpriteBatch& batch , const TileSet& tiles ,
                    int camOffX , int camOffY , int screenW , int screenH ) const;

        int W ( ) const { return m_w; }
        int H ( ) const { return m_h; }
        int At ( int x , int y ) const { return m_ids[ y * m_w + x ]; }

    private:
        int m_w = 0 , m_h = 0;
        std::vector<int> m_ids; // row-major
    };

} // namespace engine
