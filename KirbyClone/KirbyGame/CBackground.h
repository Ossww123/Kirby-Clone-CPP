#pragma once
#include "CObject.h"

class CTexture;

class CBackground : public CObject
{
public:
    CBackground();
    virtual ~CBackground();

public:
    // === 핵심 생명주기 함수들 ===
    void Update() override;
    void Render(HDC _dc) override;

public:
    // === 배경 설정 함수들 ===
    void SetBackgroundTexture(CTexture* _pTex) { m_pBackgroundTex = _pTex; }
    void SetBackgroundType(BACKGROUND_TYPE _eType) { m_eBackgroundType = _eType; }
    void SetScrollSpeed(Vec2 _vSpeed) { m_vScrollSpeed = _vSpeed; }
    void SetScrollable(bool _bScrollable) { m_bScrollable = _bScrollable; }
    void SetStageSize(Vec2 _vStageSize) { m_vStageSize = _vStageSize; }

public:
    // === 배경 정보 접근자들 ===
    CTexture* GetBackgroundTexture() const { return m_pBackgroundTex; }
    BACKGROUND_TYPE GetBackgroundType() const { return m_eBackgroundType; }
    Vec2 GetScrollSpeed() const { return m_vScrollSpeed; }
    bool IsScrollable() const { return m_bScrollable; }
    Vec2 GetStageSize() const { return m_vStageSize; }

public:
    // === 배경 타입별 설정 ===
    void SetupBackground(BACKGROUND_TYPE _eType);

private:
    // === 렌더링 내부 함수들 ===
    void RenderStaticBackground(HDC _dc);
    void RenderScrollableBackground(HDC _dc);
    void RenderParallaxBackground(HDC _dc);

private:
    // === 멤버 변수들 ===
    CTexture*       m_pBackgroundTex;   // 배경 텍스처
    BACKGROUND_TYPE m_eBackgroundType;  // 배경 타입
    Vec2            m_vScrollSpeed;     // 스크롤 속도 (패럴랙스 효과용)
    float           m_fScrollOffset;    // 현재 스크롤 오프셋
    bool            m_bScrollable;      // 스크롤 가능 여부
    Vec2            m_vStageSize;       // 스테이지 크기 (배경스크롤 범위 용)
};