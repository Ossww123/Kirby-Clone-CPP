#pragma once
#include "CObject.h"
#include <string>

enum class TILE_COLLISION : uint8_t
{
    NONE = 0,    // 빈 공간
    SOLID = 1,    // 일반 블럭(사방 막힘)
    ONEWAY_TOP = 2,    // 위에서 내려올 때만 막히는 플랫폼(상단 얇은 스트립)
    HAZARD = 4,    // 닿으면 피해(가시 등)
};

class CTile : public CObject
{
public:
    // worldLT : 타일 좌상단(월드), tileSize : 타일 픽셀 크기
    explicit CTile(const Vec2& worldLT,
        const Vec2& tileSize,
        TILE_COLLISION type = TILE_COLLISION::SOLID);
    ~CTile() override = default;

    // === CObject ===
    void Init() override;
    void Update() override;

public:
    // === 비주얼(애니) ===
    // animFile : 애니 json 경로, animName : 그 안의 애니 이름
    // visualOffset : 스프라이트만 살짝 밀 때 사용(충돌은 타일 셀 유지)
    void SetVisual(const std::wstring& animFile,
        const std::wstring& animName,
        const Vec2& visualOffset = Vec2(0.f, 0.f));
    const std::wstring& GetAnimFile() const { return m_animFile; }
    const std::wstring& GetAnimName() const { return m_animName; }
    Vec2 GetVisualOffset() const { return m_visualOffset; }

    // === 충돌/옵션 ===
    void SetCollisionType(TILE_COLLISION type);
    TILE_COLLISION GetCollisionType() const { return m_collision; }

    void  SetOneWayThicknessPx(float px);
    float GetOneWayThicknessPx() const { return m_oneWayThicknessPx; }

    void SetDamage(int dmg) { m_damage = dmg; }
    int  GetDamage() const { return m_damage; }

    void SetBlocksProjectiles(bool v) { m_blocksProjectiles = v; }
    bool BlocksProjectiles() const { return m_blocksProjectiles; }

    // === 배치/치수 ===
    Vec2 GetWorldLT()  const { return m_worldLT; }
    Vec2 GetTileSize() const { return m_tileSize; }

private:
    void buildVisual();   // Animator/애니 재설정
    void buildCollider(); // Collider 재구성(타입별)

private:
    // 배치/치수
    Vec2            m_worldLT;      // 타일 좌상단(월드)
    Vec2            m_tileSize;     // 타일 크기(px)

    // 비주얼
    std::wstring    m_animFile;
    std::wstring    m_animName;
    Vec2            m_visualOffset{ 0.f, 0.f }; // 스프라이트만 이동

    // 충돌/옵션
    TILE_COLLISION  m_collision{ TILE_COLLISION::SOLID };
    float           m_oneWayThicknessPx{ 6.f }; // 원웨이 상단 두께(px)
    int             m_damage{ 0 };              // HAZARD일 때 피해량
    bool            m_blocksProjectiles{ true };
};
