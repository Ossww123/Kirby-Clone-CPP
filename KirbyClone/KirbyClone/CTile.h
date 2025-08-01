#pragma once
#include "CObject.h"

class CCollider;
class CTexture;

class CTile : public CObject
{
private:
    OBJECT_TYPE         m_eTileType;        // 기능적 타입 (충돌, 데미지 등)
    TILE_VISUAL_TYPE    m_eVisualType;      // 시각적 타입 (렌더링용)
    CTexture* m_pTileTexture;      // 타일 전용 텍스처

    bool m_bSolid;          // 충돌 여부
    bool m_bHarmful;        // 데미지 여부 (가시 등)
    bool m_bDecorative;     // 장식용 (나무, 꽃 등 - 충돌하지 않음)

public:
    virtual void Update();
    virtual void Render(HDC _dc);

    // 기능적 타입 관련
    void SetTileType(OBJECT_TYPE _eType) { m_eTileType = _eType; }
    void SetSolid(bool _bSolid) { m_bSolid = _bSolid; }
    void SetHarmful(bool _bHarmful) { m_bHarmful = _bHarmful; }
    void SetDecorative(bool _bDecorative) { m_bDecorative = _bDecorative; }

    OBJECT_TYPE GetTileType() { return m_eTileType; }
    bool IsSolid() { return m_bSolid; }
    bool IsHarmful() { return m_bHarmful; }
    bool IsDecorative() { return m_bDecorative; }

    // 시각적 타입 관련
    void SetVisualType(TILE_VISUAL_TYPE _eVisualType) { m_eVisualType = _eVisualType; }
    void SetTileTexture(CTexture* _pTexture) { m_pTileTexture = _pTexture; }

    TILE_VISUAL_TYPE GetVisualType() { return m_eVisualType; }
    CTexture* GetTileTexture() { return m_pTileTexture; }

    // 타일 설정 자동화
    void SetupTileByVisualType(TILE_VISUAL_TYPE _eVisualType);

    virtual void OnCollisionEnter(CCollider* _pOther);

private:
    // 특수 효과 렌더링 함수들
    void RenderSpecialEffects(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderSpikes(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderWaterEffect(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderTreeDetails(HDC _dc, Vec2 vRenderPos, Vec2 vScale);
    void RenderFlowerDetails(HDC _dc, Vec2 vRenderPos, Vec2 vScale);

public:
    CTile();
    ~CTile();
};