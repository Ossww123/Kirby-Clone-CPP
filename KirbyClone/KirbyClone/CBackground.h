#pragma once
#include "CRes.h"

class CTexture;

class CBackground : public CRes
{
private:
    CTexture* m_pBackgroundTex;   // 배경 텍스처
    BACKGROUND_TYPE m_eBackgroundType;  // 배경 타입
    Vec2           m_vScrollSpeed;      // 스크롤 속도 (패럴랙스 효과용)
    float          m_fScrollOffset;     // 현재 스크롤 오프셋
    bool           m_bScrollable;       // 스크롤 가능 여부
    Vec2           m_vStageSize;        // 스테이지 크기 (배경스크롤 범위 용)

public:
    void SetBackgroundTexture(CTexture* _pTex) { m_pBackgroundTex = _pTex; }
    void SetBackgroundType(BACKGROUND_TYPE _eType) { m_eBackgroundType = _eType; }
    void SetScrollSpeed(Vec2 _vSpeed) { m_vScrollSpeed = _vSpeed; }
    void SetScrollable(bool _bScrollable) { m_bScrollable = _bScrollable; }
    void SetStageSize(Vec2 _vStageSize) { m_vStageSize = _vStageSize; }

    CTexture* GetBackgroundTexture() { return m_pBackgroundTex; }
    BACKGROUND_TYPE GetBackgroundType() { return m_eBackgroundType; }
    Vec2 GetScrollSpeed() { return m_vScrollSpeed; }
    bool IsScrollable() { return m_bScrollable; }
    Vec2 GetStageSize() { return m_vStageSize; }

    void Update();                      // 스크롤 업데이트
    void Render(HDC _dc);              // 배경 렌더링

    // 배경 타입별 기본 설정
    void SetupBackground(BACKGROUND_TYPE _eType);

public:
    CBackground();
    ~CBackground();
};