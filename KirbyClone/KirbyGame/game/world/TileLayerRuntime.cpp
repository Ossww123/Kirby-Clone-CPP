#include "game/world/TileLayerRuntime.h"

#include "engine/render/D3D11SpriteBatch.h"
#include "engine/physics/Collision.h"

namespace game {

    void TileLayerRuntime::BuildColliders ( engine::physics::CollisionSystem& sys ) const
    {
        if ( !collides ) return;
        // TileMap 쪽 정책(SOLID/ONEWAY 머지)은 그대로 재사용
        map.BuildSolidColliders ( sys , tiles );
    }

    void TileLayerRuntime::Render ( engine::D3D11SpriteBatch& batch ,
                                    int camX , int camY ,
                                    int screenW , int screenH ) const
    {
        // 레이어 오프셋만큼 카메라 기준을 보정
        const int localCamX = camX - offsetX;
        const int localCamY = camY - offsetY;

        map.Render ( batch , tiles , localCamX , localCamY , screenW , screenH );
    }

    void TileLayerRuntime::RenderScaled ( engine::D3D11SpriteBatch& batch ,
                                          int camX , int camY ,
                                          int screenW , int screenH ,
                                          int scale ) const
    {
        const int localCamX = camX - offsetX;
        const int localCamY = camY - offsetY;

        map.RenderScaled ( batch , tiles , localCamX , localCamY , screenW , screenH , scale );
    }

} // namespace game
