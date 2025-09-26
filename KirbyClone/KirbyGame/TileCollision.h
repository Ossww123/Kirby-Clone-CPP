#pragma once
#include <optional>

class CObject;
class CTile;
class CRigidBody;
class CCollider;

enum class TileKind { Solid, OneWay, PassThrough /* reserved: Slope 등 */ };

struct TileCollisionOpts {
    bool  enableOneWay = true;     // 원웨이(내려갈 때만 충돌)
    float oneWayVelY = -50.f;    // 아래로 이속이 이 값 이하일 때만 원웨이 충돌
    float skin = 0.5f;     // 분리 후 겹침 방지 여유
};

struct TileContactInfo {
    bool  wall = false;
    bool  ground = false;
    bool  ceiling = false;
    Vec2  normal{ 0.f, 0.f };        // 접촉 법선 (벽: (-/+1,0), 지면:(0,1), 천장:(0,-1))
    Vec2  separation{ 0.f, 0.f };    // 적용된 보정 벡터
    CTile* tile = nullptr;         // (선택) 필요 시 참조
};

namespace TileCollision {
    // actor: 동적 오브젝트(몬스터/플레이어 등), tileObj: 타일 오브젝트
    // 반환: 충돌/분리 수행 여부
    bool ResolveAgainstTile(CObject& actor, CObject& tileObj,
        const TileCollisionOpts& opts,
        TileContactInfo& out);
}
