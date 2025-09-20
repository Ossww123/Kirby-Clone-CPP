#include "gamePCH.h"
#include "CBackground.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CCore.h"
#include "CTimeMgr.h"

CBackground::CBackground()
    : m_pBackgroundTex(nullptr)
    , m_eBackgroundType(BACKGROUND_TYPE::BACKGROUND1)
    , m_vScrollSpeed(Vec2(0.5f, 0.f))
    , m_fScrollOffset(0.f)
    , m_bScrollable(true)
    , m_vStageSize(Vec2(4096.f, 640.f))  // Default stage size
{
}

CBackground::~CBackground()
{
}

void CBackground::Update()
{
    if (m_bScrollable && nullptr != m_pBackgroundTex)
    {
        // Get camera position and screen resolution
        Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();
        Vec2 vResolution = CCore::GetInst()->GetResolution();
        
        // Get background texture size (scaled)
        float fScale = CCore::GetPixelScale();
        UINT bgWidth = m_pBackgroundTex->GetWidth();
        UINT scaledBgWidth = (UINT)(bgWidth * fScale);
        
        // Calculate proportional offset based on camera position relative to stage
        // Formula: bgOffsetX = (cameraX / stageWidth) * (bgWidth - screenWidth)
        if (m_vStageSize.x > 0 && scaledBgWidth > vResolution.x)
        {
            float cameraRatio = vCameraPos.x / m_vStageSize.x;
            m_fScrollOffset = cameraRatio * (scaledBgWidth - vResolution.x);
        }
        else
        {
            m_fScrollOffset = 0.f;
        }
    }
}

void CBackground::Render(HDC _dc)
{
    if (nullptr == m_pBackgroundTex)
        return;

    Vec2 vResolution = CCore::GetInst()->GetResolution();
    UINT bgWidth = m_pBackgroundTex->GetWidth();
    UINT bgHeight = m_pBackgroundTex->GetHeight();

    // 4배 스케일 적용
    float fScale = CCore::GetPixelScale();
    UINT scaledWidth = (UINT)(bgWidth * fScale);
    UINT scaledHeight = (UINT)(bgHeight * fScale);

    if (m_bScrollable)
    {
        // Proportional background rendering - show portion of single image
        // Calculate source rectangle based on scroll offset
        int srcX = (int)(m_fScrollOffset / fScale);
        int srcWidth = (int)(vResolution.x / fScale);
        int srcHeight = bgHeight;
        
        // Clamp source rectangle to background texture bounds
        if (srcX < 0) srcX = 0;
        if (srcX + srcWidth > (int)bgWidth) srcWidth = bgWidth - srcX;
        if (srcWidth < 0) srcWidth = 0;
        
        // Render the portion of background that corresponds to current camera position
        StretchBlt(_dc,
            0, 0,
            (int)vResolution.x, (int)vResolution.y,
            m_pBackgroundTex->GetDC(),
            srcX, 0,
            srcWidth, srcHeight,
            SRCCOPY);
    }
    else
    {
        // Static background - stretch entire image to fill screen
        StretchBlt(_dc,
            0, 0,
            (int)vResolution.x, (int)vResolution.y,
            m_pBackgroundTex->GetDC(),
            0, 0,
            bgWidth, bgHeight,
            SRCCOPY);
    }
}

void CBackground::SetupBackground(BACKGROUND_TYPE _eType)
{
    m_eBackgroundType = _eType;

    switch (_eType)
    {
    case BACKGROUND_TYPE::BACKGROUND1:
        m_vScrollSpeed = Vec2(0.3f, 0.f);   // 천천히 패럴렉스
        m_bScrollable = true;
        break;

    case BACKGROUND_TYPE::BACKGROUND2:
        m_vScrollSpeed = Vec2(0.4f, 0.f);   // 중간 속도 패럴렉스
        m_bScrollable = true;
        break;

    case BACKGROUND_TYPE::BACKGROUND3:
        m_vScrollSpeed = Vec2(0.2f, 0.f);   // 더 느린 패럴렉스 (멀리 있는 배경)
        m_bScrollable = true;
        break;

    default:
        m_vScrollSpeed = Vec2(0.5f, 0.f);
        m_bScrollable = true;
        break;
    }
}