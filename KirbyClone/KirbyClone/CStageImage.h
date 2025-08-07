#pragma once
#include "CRes.h"

class CTexture;

// 스테이지 전체 이미지를 관리하는 클래스
class CStageImage : public CRes
{
private:
    CTexture* m_pStageTexture;          // 스테이지 전체 이미지 텍스처
    STAGE_IMAGE_TYPE m_eStageType;      // 스테이지 이미지 타입
    Vec2 m_vImageSize;                  // 이미지 크기
    Vec2 m_vRenderOffset;               // 렌더링 오프셋 (필요시)
    bool m_bScrollWithCamera;           // 카메라와 함께 스크롤 여부

public:
    // Getter/Setter
    void SetStageTexture(CTexture* _pTex) { m_pStageTexture = _pTex; }
    void SetStageType(STAGE_IMAGE_TYPE _eType) { m_eStageType = _eType; }
    void SetImageSize(Vec2 _vSize) { m_vImageSize = _vSize; }
    void SetRenderOffset(Vec2 _vOffset) { m_vRenderOffset = _vOffset; }
    void SetScrollWithCamera(bool _bScroll) { m_bScrollWithCamera = _bScroll; }

    CTexture* GetStageTexture() { return m_pStageTexture; }
    STAGE_IMAGE_TYPE GetStageType() { return m_eStageType; }
    Vec2 GetImageSize() { return m_vImageSize; }
    Vec2 GetRenderOffset() { return m_vRenderOffset; }
    bool IsScrollWithCamera() { return m_bScrollWithCamera; }

    // 핵심 기능
    void Update();                      // 업데이트 (필요시)
    void Render(HDC _dc);              // 스테이지 이미지 렌더링
    void RenderWithAlpha(HDC _dc, float _fAlpha = 1.0f);  // 투명도 조절 렌더링

    // 스테이지 타입별 기본 설정
    void SetupStageImage(STAGE_IMAGE_TYPE _eType);

public:
    CStageImage();
    ~CStageImage();
};