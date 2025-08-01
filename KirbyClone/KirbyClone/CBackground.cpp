#include "pch.h"
#include "CBackground.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CCore.h"
#include "CTimeMgr.h"

CBackground::CBackground()
    : m_pBackgroundTex(nullptr)
    , m_eBackgroundType(BACKGROUND_TYPE::GREEN_HILL)
    , m_vScrollSpeed(Vec2(0.5f, 0.f))
    , m_fScrollOffset(0.f)
    , m_bScrollable(true)
{
}

CBackground::~CBackground()
{
}

void CBackground::Update()
{
    if (m_bScrollable)
    {
        // 카메라 위치에 따른 패럴랙스 스크롤링
        Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();
        m_fScrollOffset = vCameraPos.x * m_vScrollSpeed.x;
    }
}

void CBackground::Render(HDC _dc)
{
    if (nullptr == m_pBackgroundTex)
        return;

    Vec2 vResolution = CCore::GetInst()->GetResolution();
    UINT bgWidth = m_pBackgroundTex->GetWidth();
    UINT bgHeight = m_pBackgroundTex->GetHeight();

    if (m_bScrollable)
    {
        // 무한 스크롤링을 위한 배경 반복 렌더링
        int startX = (int)(-m_fScrollOffset) % bgWidth;
        if (startX > 0) startX -= bgWidth;

        for (int x = startX; x < (int)vResolution.x; x += bgWidth)
        {
            // 배경을 화면 크기에 맞춰 스케일링하여 렌더링
            StretchBlt(_dc,
                x, 0,
                bgWidth, (int)vResolution.y,
                m_pBackgroundTex->GetDC(),
                0, 0,
                bgWidth, bgHeight,
                SRCCOPY);
        }
    }
    else
    {
        // 고정 배경 - 화면 전체에 스트레치
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
    case BACKGROUND_TYPE::GREEN_HILL:
        m_vScrollSpeed = Vec2(0.3f, 0.f);   // 느린 패럴랙스
        m_bScrollable = true;
        break;

    case BACKGROUND_TYPE::RAINBOW_CASTLE:
        m_vScrollSpeed = Vec2(0.2f, 0.f);   // 더 느린 패럴랙스 (멀리 있는 느낌)
        m_bScrollable = true;
        break;

    default:
        m_vScrollSpeed = Vec2(0.5f, 0.f);
        m_bScrollable = true;
        break;
    }
}