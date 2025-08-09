#pragma once
#include "CObject.h"

class CCollider;
class CTexture;

class CTile : public CObject
{
private:
    // === 충돌체 전용 데이터 ===
    OBJECT_TYPE m_eTileType;            // 기본 오브젝트 타입 (호환성용)
    COLLISION_TYPE m_eCollisionType;    // 새로운 충돌체 타입

    // 충돌 속성들
    bool m_bSolid;                      // 단단한 충돌 (통과 불가)
    bool m_bHarmful;                    // 데미지를 주는가
    bool m_bOneWay;                     // 일방통행인가 (위에서만 충돌)
    bool m_bDecorative;                 // 장식용인가 (충돌 없음)

    // 렌더링 관련 (에디터용)
    COLORREF m_displayColor;            // 에디터에서 표시할 색깔
    bool m_bShowInEditor;               // 에디터에서 표시 여부

    // 기본 텍스처 관련 (사용 안함, 호환성용으로만 유지)
    TILE_VISUAL_TYPE m_eVisualType;     // 기본 시각적 타입 (더 이상 사용 안함)
    CTexture* m_pTileTexture;           // 더 이상 사용 안함 (nullptr로 유지)

public:
    CTile();
    ~CTile();

    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // === 충돌체 타입 관련 ===
    void SetCollisionType(COLLISION_TYPE _eType);
    COLLISION_TYPE GetCollisionType() const { return m_eCollisionType; }

    // === 속성 설정/조회 ===
    void SetSolid(bool _bSolid) { m_bSolid = _bSolid; }
    bool IsSolid() const { return m_bSolid; }

    void SetHarmful(bool _bHarmful) { m_bHarmful = _bHarmful; }
    bool IsHarmful() const { return m_bHarmful; }

    void SetOneWay(bool _bOneWay) { m_bOneWay = _bOneWay; }
    bool IsOneWay() const { return m_bOneWay; }

    void SetDecorative(bool _bDecorative) { m_bDecorative = _bDecorative; }
    bool IsDecorative() const { return m_bDecorative; }

    // === 에디터 렌더링 관련 ===
    void SetDisplayColor(COLORREF _color) { m_displayColor = _color; }
    COLORREF GetDisplayColor() const { return m_displayColor; }

    void SetShowInEditor(bool _bShow) { m_bShowInEditor = _bShow; }
    bool ShouldShowInEditor() const { return m_bShowInEditor; }

    // === 기본 호환성 유지 함수들 (빈 구현) ===
    void SetVisualType(TILE_VISUAL_TYPE _eType) { m_eVisualType = _eType; } // 호환성용
    TILE_VISUAL_TYPE GetVisualType() const { return m_eVisualType; }        // 호환성용

    void SetTileType(OBJECT_TYPE _eType) { m_eTileType = _eType; }          // 호환성용
    OBJECT_TYPE GetTileType() const { return m_eTileType; }                 // 호환성용

    void SetTileTexture(CTexture* _pTexture) { /* 더 이상 사용 안함 */ }     // 호환성용

private:
    // === 내부 헬퍼 함수들 ===
    void SetupCollisionDefaults(COLLISION_TYPE _eType);
    void UpdateCollisionProperties();
    void RenderCollisionBox(HDC _dc);
    void RenderSpecialIndicators(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderCollisionInfo(HDC _dc);
};