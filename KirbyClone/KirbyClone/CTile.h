#pragma once
#include "CObject.h"

class CCollider;
class CTexture;

class CTile : public CObject
{
public:
    // === 핵심 생명주기 함수들 ===
    CTile();
    ~CTile();

    void Update() override;
    void Render(HDC _dc) override;

public:
    // === 충돌 타입 관리 ===
    void SetCollisionType(COLLISION_TYPE _eType);
    COLLISION_TYPE GetCollisionType() const { return m_eCollisionType; }

private:
    void SetupCollisionDefaults(COLLISION_TYPE _eType);
    void UpdateCollisionProperties();

public:
    // === 속성 설정/조회 ===
    void SetSolid(bool _bSolid) { m_bSolid = _bSolid; }
    bool IsSolid() const { return m_bSolid; }

    void SetHarmful(bool _bHarmful) { m_bHarmful = _bHarmful; }
    bool IsHarmful() const { return m_bHarmful; }

    void SetOneWay(bool _bOneWay) { m_bOneWay = _bOneWay; }
    bool IsOneWay() const { return m_bOneWay; }

    bool IsDecorative() const { return !m_bSolid && !m_bHarmful && !m_bOneWay; }

public:
    // === 에디터 렌더링 관리 ===
    void SetDisplayColor(COLORREF _color) { m_displayColor = _color; }
    COLORREF GetDisplayColor() const { return m_displayColor; }

private:
    void RenderCollisionBox(HDC _dc);
    void RenderSpecialIndicators(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderCollisionInfo(HDC _dc);

    // === 타일 타입별 렌더링 ===
    void RenderTransparentBlock(HDC _dc);
    void RenderCommonElements(HDC _dc);

public:
    // === 기본 호환성 유지 함수들 (향후 구현) ===
    void SetVisualType(TILE_VISUAL_TYPE _eType) { m_eVisualType = _eType; }
    TILE_VISUAL_TYPE GetVisualType() const { return m_eVisualType; }

    void SetTileType(OBJECT_TYPE _eType) { m_eTileType = _eType; }
    OBJECT_TYPE GetTileType() const { return m_eTileType; }

    void SetTileTexture(CTexture* _pTexture) { /* 향후 구현 */ }

private:
    // === 충돌 및 타일 데이터 ===
    OBJECT_TYPE         m_eTileType;            // 기본 오브젝트 타입
    COLLISION_TYPE      m_eCollisionType;       // 상호작용 충돌 타입

    // === 충돌 속성들 ===
    bool                m_bSolid;               // 단단한 충돌 (통과 불가)
    bool                m_bHarmful;             // 데미지를 주는가
    bool                m_bOneWay;              // 일방통행인가 (위에서만 충돌)

    // === 에디터 렌더링 ===
    COLORREF            m_displayColor;         // 에디터에서 표시할 색깔

    // === 기본 텍스처 관리 (미사용) ===
    TILE_VISUAL_TYPE    m_eVisualType;          // 기본 시각적 타입
    CTexture*           m_pTileTexture;         // 대체 이미지 (nullptr 유지)
};