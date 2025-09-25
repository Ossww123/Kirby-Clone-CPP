#include "gamePCH.h"
#include "TileCollision.h"
#include "CObject.h"
#include "CTile.h"
#include "CCollider.h"
#include "CRigidBody.h"

static inline bool IsTileSolid(const CObject& tileObj) {
    // 그룹으로 구분 (권장)
    return tileObj.GetGroup() == GROUP_TYPE::TILE;
    // 필요하면 OBJECT_TYPE로 세부 타입 분기 추가
}

static inline TileKind GetTileKind(const CObject& tileObj) {
    // 프로젝트 규칙에 맞게 확장
    // 예: 특정 OBJECT_TYPE 범위는 OneWay
    // if (tileObj.GetType() == OBJECT_TYPE::TILE_ONEWAY) return TileKind::OneWay;
    return TileKind::Solid;
}

static inline Vec2 ColliderPos(const CObject& o) { return o.GetPos(); }
static inline Vec2 ColliderSize(const CObject& o) { return o.GetCollider() ? o.GetCollider()->GetScale() : o.GetScale(); }

bool TileCollision::ResolveAgainstTile(CObject& actor, CObject& tileObj,
    const TileCollisionOpts& opts,
    TileContactInfo& out)
{
    if (!actor.GetCollider() || !tileObj.GetCollider()) return false;
    if (!IsTileSolid(tileObj)) return false;

    const TileKind kind = GetTileKind(tileObj);

    Vec2 aPos = ColliderPos(actor);
    Vec2 aSz = ColliderSize(actor);
    Vec2 tPos = ColliderPos(tileObj);
    Vec2 tSz = ColliderSize(tileObj);

    // 반폭/반높이
    const float aHalfX = aSz.x * 0.5f;
    const float aHalfY = aSz.y * 0.5f;
    const float tHalfX = tSz.x * 0.5f;
    const float tHalfY = tSz.y * 0.5f;

    const float dx = aPos.x - tPos.x;
    const float dy = aPos.y - tPos.y;
    const float overlapX = (aHalfX + tHalfX) - fabsf(dx);
    const float overlapY = (aHalfY + tHalfY) - fabsf(dy);

    if (overlapX <= 0.f || overlapY <= 0.f)
        return false; // 겹치지 않음

    // 원웨이: 위에서 내려올 때(velocity.y <= threshold)만 Y축 양수 분리 허용
    if (kind == TileKind::OneWay) {
        CRigidBody* rb = actor.GetRigidBody();
        const float vy = rb ? rb->GetVelocity().y : 0.f;
        const bool comingDown = (vy <= opts.oneWayVelY);
        const bool actorAbove = (aPos.y < tPos.y); // 타일 위에 위치
        if (!(opts.enableOneWay && comingDown && actorAbove)) {
            return false; // 원웨이 조건 불충족 → 충돌 무시
        }
    }

    // 더 작은 축으로 분리
    Vec2 sep{ 0.f, 0.f };
    Vec2 n{ 0.f, 0.f };
    TileContactInfo info;

    if (overlapX < overlapY) {
        // 수평 분리
        if (dx < 0) { sep.x = -overlapX - opts.skin; n.x = -1.f; } // 타일이 오른쪽
        else { sep.x = overlapX + opts.skin; n.x = 1.f; } // 타일이 왼쪽
        info.wall = true;
    }
    else {
        // 수직 분리
        if (dy < 0) { sep.y = -overlapY - opts.skin; n.y = -1.f; info.ceiling = true; }
        else { sep.y = overlapY + opts.skin; n.y = 1.f; info.ground = true; }
    }

    // 위치 보정 적용
    actor.SetPos(Vec2{ aPos.x + sep.x, aPos.y + sep.y });

    // 속도 보정 + ground 플래그
    if (CRigidBody* rb = actor.GetRigidBody()) {
        Vec2 v = rb->GetVelocity();
        if (info.wall)    v.x = 0.f;
        if (info.ground || info.ceiling) v.y = 0.f;
        if (info.ground)  rb->SetGround(true);
        if (info.ceiling) rb->SetGround(false); // 천장 찍으면 지면X
        rb->SetVelocity(v);
    }

    info.normal = n;
    info.separation = sep;
    info.tile = dynamic_cast<CTile*>(&tileObj);
    out = info;
    return true;
}
