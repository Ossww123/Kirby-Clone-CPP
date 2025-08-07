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
        return;

    Vec2 vResolution = CCore::GetInst()->GetResolution();
    UINT imageWidth = m_pStageTexture->GetWidth();
    UINT imageHeight = m_pStageTexture->GetHeight();

    // 렌더링 위치 계산
    Vec2 vRenderPos = m_vRenderOffset;

    if (m_bScrollWithCamera)
    {
        // 카메라 위치를 고려한 렌더링 (스테이지는 보통 월드 좌표계 기준)
        vRenderPos = CCamera::GetInst()->GetRenderPos(m_vRenderOffset);
    }

    // 알파 채널 지원 여부에 따라 적절한 렌더링 방식 선택
    if (m_vImageSize.x > 0 && m_vImageSize.y > 0)
    {
        // 지정된 크기로 렌더링
        if (m_pStageTexture->HasAlpha())
        {
            // 32비트 알파 채널 렌더링
            m_pStageTexture->RenderWithAlpha(_dc,
                (int)vRenderPos.x, (int)vRenderPos.y,
                (int)m_vImageSize.x, (int)m_vImageSize.y);
        }
        else
        {
            // 기존 방식 (불투명 렌더링)
            StretchBlt(_dc,
                (int)vRenderPos.x, (int)vRenderPos.y,
                (int)m_vImageSize.x, (int)m_vImageSize.y,
                m_pStageTexture->GetDC(),
                0, 0,
                imageWidth, imageHeight,
                SRCCOPY);
        }
    }
    else
    {
        // 원본 크기로 렌더링
        if (m_pStageTexture->HasAlpha())
        {
            // 32비트 알파 채널 렌더링
            m_pStageTexture->RenderWithAlpha(_dc,
                (int)vRenderPos.x, (int)vRenderPos.y,
                imageWidth, imageHeight);
        }
        else
        {
            // 기존 방식 (불투명 렌더링)
            BitBlt(_dc,
                (int)vRenderPos.x, (int)vRenderPos.y,
                imageWidth, imageHeight,
                m_pStageTexture->GetDC(),
                0, 0,
                SRCCOPY);
        }
    }
}

void CStageImage::RenderWithAlpha(HDC _dc, float _fAlpha)
{
    if (nullptr == m_pStageTexture)
        return;

    if (!m_pStageTexture->HasAlpha())
    {
        // 알파 채널이 없으면 기본 렌더링
        Render(_dc);
        return;
    }

    Vec2 vRenderPos = m_vRenderOffset;

    if (m_bScrollWithCamera)
    {
        vRenderPos = CCamera::GetInst()->GetRenderPos(m_vRenderOffset);
    }

    UINT imageWidth = m_pStageTexture->GetWidth();
    UINT imageHeight = m_pStageTexture->GetHeight();

    if (m_vImageSize.x > 0 && m_vImageSize.y > 0)
    {
        // 지정된 크기로 알파 렌더링
        m_pStageTexture->RenderWithAlpha(_dc,
            (int)vRenderPos.x, (int)vRenderPos.y,
            (int)m_vImageSize.x, (int)m_vImageSize.y,
            _fAlpha);
    }
    else
    {
        // 원본 크기로 알파 렌더링
        m_pStageTexture->RenderWithAlpha(_dc,
            (int)vRenderPos.x, (int)vRenderPos.y,
            imageWidth, imageHeight,
            _fAlpha);
    }
}

void CStageImage::SetupStageImage(STAGE_IMAGE_TYPE _eType)
{
    m_eStageType = _eType;

    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:
        m_bScrollWithCamera = true;
        m_vRenderOffset = Vec2(0.f, 0.f);
        break;

    case STAGE_IMAGE_TYPE::STAGE_02:
        m_bScrollWithCamera = true;
        m_vRenderOffset = Vec2(0.f, 0.f);
        break;

    case STAGE_IMAGE_TYPE::CUSTOM:
        m_bScrollWithCamera = true;
        m_vRenderOffset = Vec2(0.f, 0.f);
        break;

    default:
        m_bScrollWithCamera = true;
        m_vRenderOffset = Vec2(0.f, 0.f);
        break;
    }

    // 텍스처가 로드된 후에 이미지 크기 자동 설정
    if (m_pStageTexture)
    {
        m_vImageSize = Vec2((float)m_pStageTexture->GetWidth(), (float)m_pStageTexture->GetHeight());
    }
}