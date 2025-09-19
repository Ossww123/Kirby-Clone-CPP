#pragma once
#include "CObject.h"

class CCollider;
class CTexture;

class CTile : public CObject
{
public:
    // === 기본 생명주기 함수들 ===
    CTile ( );
    ~CTile ( );

    void Update ( ) override;
    void Render ( HDC _dc ) override;

public:
    // === 충돌 타입 관리 ===
    void SetCollisionType ( COLLISION_TYPE _eType );
    COLLISION_TYPE GetCollisionType ( ) const { return m_eCollisionType; }

private:
    void SetupCollisionDefaults ( COLLISION_TYPE _eType );
    void UpdateCollisionProperties ( );

public:
    // === 속성 설정/조회 ===
    void SetSolid ( bool _bSolid ) { m_bSolid = _bSolid; }
    bool IsSolid ( ) const { return m_bSolid; }

    void SetHarmful ( bool _bHarmful ) { m_bHarmful = _bHarmful; }
    bool IsHarmful ( ) const { return m_bHarmful; }

    void SetOneWay ( bool _bOneWay ) { m_bOneWay = _bOneWay; }
    bool IsOneWay ( ) const { return m_bOneWay; }

    bool IsDecorative ( ) const { return !m_bSolid && !m_bHarmful && !m_bOneWay; }

public:
    // === 디버그 렌더링 관련 ===
    void SetDisplayColor ( COLORREF _color ) { m_displayColor = _color; }
    COLORREF GetDisplayColor ( ) const { return m_displayColor; }

private:
    void RenderCollisionBox ( HDC _dc );
    void RenderSpecialIndicators ( HDC _dc , Vec2 vRenderPos , Vec2 vScale );
    void RenderCollisionInfo ( HDC _dc );

    // === 타일 타입별 렌더링 ===
    void RenderTransparentBlock ( HDC _dc );
    void RenderTriggerTile ( HDC _dc );
    void RenderCommonElements ( HDC _dc );

public:
    // === 기본 호환성 유지 함수들 (현재 사용) ===
    void SetVisualType ( TILE_VISUAL_TYPE _eType );
    TILE_VISUAL_TYPE GetVisualType ( ) const { return m_eVisualType; }

    void SetTileType ( OBJECT_TYPE _eType ) { m_eTileType = _eType; }
    OBJECT_TYPE GetTileType ( ) const { return m_eTileType; }

    void SetTileTexture ( CTexture* _pTexture ) { /* 현재 미구현 */ }

    // === 보스전 타입 특수 효과용 ===
    void SetBossLockPosition ( Vec2 _vPos ) { m_vBossLockPos = _vPos; }
    Vec2 GetBossLockPosition ( ) const { return m_vBossLockPos; }

    // === Trigger activation state ===
    void SetTriggerActive ( bool _bActive ) { m_bTriggerActive = _bActive; }
    bool IsTriggerActive ( ) const { return m_bTriggerActive; }

private:
    // === 충돌 및 타일 데이터 ===
    OBJECT_TYPE         m_eTileType;            // 기본 오브젝트 타입
    COLLISION_TYPE      m_eCollisionType;       // 상호작용 충돌 타입

    // === 충돌 속성들 ===
    bool                m_bSolid;               // 단단한 충돌 (통과 불가)
    bool                m_bHarmful;             // 해로운지 있는가
    bool                m_bOneWay;              // 일방향인가 (아래에서 충돌)

    // === 디버그 렌더링 ===
    COLORREF            m_displayColor;         // 에디터에서 표시될 색상

    // === 기본 텍스처 관련 (미사용) ===
    TILE_VISUAL_TYPE    m_eVisualType;          // 기본 비주얼 타입
    CTexture* m_pTileTexture;         // 실제 이미지 (nullptr 상태)

    // === 보스전 타입 특수 변수들 ===
    Vec2                m_vBossLockPos;         // 보스전 타입 발동 시점에 저장할 카메라 위치
    bool                m_bTriggerActive;       // Trigger activation state (default: true)
};