#pragma once

class CPlayer;
class CCollider;
class CObject;
class CTile;
class CMonster;
class CRigidBody;

class CPlayerCollisionSystem
{
public:
    CPlayerCollisionSystem(CPlayer* _pOwner);
    ~CPlayerCollisionSystem();

    // === 초기화 ===
    void Init();

    // === 충돌 처리 메인 인터페이스 ===
    void HandleCollisionEnter(CCollider* _pOther);
    void HandleCollision(CCollider* _pOther);
    void HandleCollisionExit(CCollider* _pOther);

private:
    // === 충돌 상수들 ===
    static constexpr float TILE_COLLISION_THRESHOLD = 8.f;      // 타일 충돌 감지 임계값
    static constexpr float POSITION_CORRECTION_THRESHOLD = 5.f; // 위치 보정 임계값
    static constexpr float JUMP_VELOCITY_THRESHOLD = -50.f;     // 점프 속도 임계값

    // === 타입별 충돌 처리 함수들 ===
    void HandleTileCollisionEnter(CTile* _pTile);
    void HandleTileCollision(CTile* _pTile);
    void HandleTileCollisionExit(CTile* _pTile);

    void HandleMonsterCollisionEnter(CMonster* _pMonster);
    void HandleItemCollisionEnter(CObject* _pItem);
    void HandleSpecialObjectCollisionEnter(CObject* _pSpecialObject);

    // === 타일 충돌 세부 처리 ===
    bool ShouldSetGroundState(CTile* _pTile) const;
    void CorrectPlayerPosition(CTile* _pTile);
    void UpdateGroundState(CTile* _pTile);
    void ResetVerticalVelocity();

    // === 충돌 계산 헬퍼 함수들 ===
    bool IsPlayerAboveTile(CTile* _pTile) const;
    float GetTileTopPosition(CTile* _pTile) const;
    float GetPlayerBottomPosition() const;
    Vec2 GetPlayerColliderScale() const;
    Vec2 GetTileColliderScale(CTile* _pTile) const;

    // === 무적 상태 체크 ===
    bool IsInvincibleState() const;

    // === 오브젝트 타입 유틸리티 함수들 ===
    static bool IsMonsterType(OBJECT_TYPE _eType);
    static bool IsTileType(OBJECT_TYPE _eType);
    static bool IsItemType(OBJECT_TYPE _eType);
    static bool IsSpecialObjectType(OBJECT_TYPE _eType);

private:
    // === 멤버 변수 ===
    CPlayer* m_pOwner;              // 플레이어 참조
};