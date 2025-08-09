#pragma once
#include "CRes.h"

// 전방 선언
class CTexture;

// 스테이지 전체 이미지를 관리하는 클래스
class CStageImage : public CRes
{
public:
    // === 생명주기 함수 ===
    CStageImage();
    virtual ~CStageImage();

public:
    // === 핵심 기능 ===
    void Update();
    void Render(HDC _dc);

public:
    // === 스테이지 설정 ===
    void SetupStageImage(STAGE_IMAGE_TYPE _eType);
    void SetImageToBottomLeft();

public:
    // === 텍스처 관리 ===
    void SetStageTexture(CTexture* _pTex) { m_pStageTexture = _pTex; }
    CTexture* GetStageTexture() const { return m_pStageTexture; }

public:
    // === 스테이지 정보 접근자 ===
    void SetStageType(STAGE_IMAGE_TYPE _eType) { m_eStageType = _eType; }
    STAGE_IMAGE_TYPE GetStageType() const { return m_eStageType; }

public:
    // === 이미지 속성 관리 ===
    void SetImageSize(Vec2 _vSize) { m_vImageSize = _vSize; }
    Vec2 GetImageSize() const { return m_vImageSize; }

    void SetRenderOffset(Vec2 _vOffset) { m_vRenderOffset = _vOffset; }
    Vec2 GetRenderOffset() const { return m_vRenderOffset; }

    void SetScrollWithCamera(bool _bScroll) { m_bScrollWithCamera = _bScroll; }
    bool IsScrollWithCamera() const { return m_bScrollWithCamera; }

private:
    // === 멤버 변수들 ===
    CTexture* m_pStageTexture;    // 스테이지 전체 이미지 텍스처
    STAGE_IMAGE_TYPE m_eStageType;       // 스테이지 이미지 타입
    Vec2             m_vImageSize;       // 이미지 크기
    Vec2             m_vRenderOffset;    // 렌더링 오프셋 (필요시)
    bool             m_bScrollWithCamera; // 카메라와 함께 스크롤 여부
};