#pragma once

class CPlayer;
class CCollider;
class CObject;
class CTile;
class CMonster;

class CPlayerCollisionSystem
{
public:
    CPlayerCollisionSystem(CPlayer* _pOwner);
    ~CPlayerCollisionSystem();

public:
    // === 핵심 인터페이스 ===
    void Init();
    void UpdateGroundState();

    // === 충돌 처리 메인 인터페이스 ===
    void HandleCollisionEnter(CCollider* _pOther);
    void HandleCollision(CCollider* _pOther);
    void HandleCollisionExit(CCollider* _pOther);

private:
    // === 충돌 상수들 ===
    static constexpr float TILE_COLLISION_THRESHOLD = 8.f;
    static constexpr float POSITION_CORRECTION_THRESHOLD = 5.f;

    // === 타입별 충돌 처리 ===
    void HandleTileCollisionEnter(CTile* _pTile);
    void HandleTileCollision(CTile* _pTile);
    void HandleTileCollisionExit(CTile* _pTile);
    void HandleMonsterCollisionEnter(CMonster* _pMonster);
    void HandleItemCollisionEnter(CObject* _pItem);
    void HandleSpecialObjectCollisionEnter(CObject* _pSpecialObject);

    // === 타일 충돌 세부 처리 ===
    bool ShouldSetGroundState(CTile* _pTile) const;
    void CorrectPlayerPosition(CTile* _pTile);
    void SetGroundState(CTile* _pTile);
    void ResetVerticalVelocity();

    // === 헬퍼 함수들 ===
    bool IsPlayerAboveTile(CTile* _pTile) const;
    float GetTileTopPosition(CTile* _pTile) const;
    float GetPlayerBottomPosition() const;
    Vec2 GetPlayerColliderScale() const;
    Vec2 GetTileColliderScale(CTile* _pTile) const;

    // === 유틸리티 함수들 ===
    bool IsInvincibleState() const;
    bool IsCollidingWithOtherSolidTiles(CTile* _excludeTile) const;
    static bool IsMonsterType(OBJECT_TYPE _eType);
    static bool IsTileType(OBJECT_TYPE _eType);
    static bool IsItemType(OBJECT_TYPE _eType);
    static bool IsSpecialObjectType(OBJECT_TYPE _eType);

private:
    // === 멤버 변수들 ===
    CPlayer* m_pOwner;
    bool m_bNeedGroundCheck;
    CTile* m_pExitingTile;
};