#include "pch.h"
#include "CStageImage.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CCore.h"

CStageImage::CStageImage()
    : m_pStageTexture(nullptr)
    , m_eStageType(STAGE_IMAGE_TYPE::STAGE_01)
    , m_vImageSize(Vec2(0.f, 0.f))
    , m_vRenderOffset(Vec2(0.f, 0.f))
    , m_bScrollWithCamera(true)
{
}

CStageImage::~CStageImage()
{
    // 텍스처는 CResMgr에서 관리하므로 여기서 삭제하지 않음
    m_pStageTexture = nullptr;
}

void CStageImage::Update()
{
    // 기본적으로 스테이지 이미지는 정적이지만, 
    // 필요시 애니메이션이나 특수 효과를 위한 업데이트 로직 추가 가능
}

void CStageImage::Render(HDC _dc)
{
    if (nullptr == m_pStageTexture)
    {
        return;
    }

    UINT imageWidth = m_pStageTexture->GetWidth();
    UINT imageHeight = m_pStageTexture->GetHeight();

    // 렌더링 위치 계산
    Vec2 vRenderPos = m_vRenderOffset;

    if (m_bScrollWithCamera)
    {
        // 카메라 위치를 고려한 렌더링 (스테이지는 보통 월드 좌표계 기준)
        vRenderPos = CCamera::GetInst()->GetRenderPos(m_vRenderOffset);
    }

    // 24비트 BMP + 마젠타 컬러키 방식으로 렌더링
    if (m_vImageSize.x > 0 && m_vImageSize.y > 0)
    {
        // 지정된 크기로 렌더링
        m_pStageTexture->RenderWithColorKey(_dc,
            (int)vRenderPos.x, (int)vRenderPos.y,
            (int)m_vImageSize.x, (int)m_vImageSize.y,
            RGB(255, 0, 255)); // 마젠타 컬러키
    }
    else
    {
        // 원본 크기로 렌더링
        m_pStageTexture->RenderWithColorKey(_dc,
            (int)vRenderPos.x, (int)vRenderPos.y,
            imageWidth, imageHeight,
            RGB(255, 0, 255)); // 마젠타 컬러키
    }
}

void CStageImage::SetupStageImage(STAGE_IMAGE_TYPE _eType)
{
    m_eStageType = _eType;

    // 스테이지별 기본 설정
    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:
        // Green Hill Stage 설정
        SetImageToBottomLeft();
        SetScrollWithCamera(true);
        break;

    case STAGE_IMAGE_TYPE::STAGE_02:
        // Castle Stage 설정
        SetImageToBottomLeft();
        SetScrollWithCamera(true);
        break;

    default:
        // 기본 설정
        SetImageToBottomLeft();
        SetScrollWithCamera(true);
        break;
    }
}

void CStageImage::SetImageToBottomLeft()
{
    if (!m_pStageTexture)
        return;

    // 텍스처를 화면 왼쪽 하단에 배치
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    UINT imageHeight = m_pStageTexture->GetHeight();

    // 이미지를 화면 하단에 맞춰 배치 (y좌표는 화면 하단 - 이미지 높이)
    m_vRenderOffset = Vec2(0.f, vResolution.y - imageHeight);
}
