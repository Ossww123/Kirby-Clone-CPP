#include "gamePCH.h"
#include "CBackground.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CCore.h"
#include "CTimeMgr.h"

CBackground::CBackground()
    : CObject(OBJECT_TYPE::OBJECT_BACKGROUND)
    , m_pBackgroundTex(nullptr)
    , m_eBackgroundType(BACKGROUND_TYPE::STATIC)
    , m_vScrollSpeed(Vec2(0.f, 0.f))
    , m_fScrollOffset(0.f)
    , m_bScrollable(false)
    , m_vStageSize(Vec2(0.f, 0.f))
{
    // 배경은 화면 전체 크기로 설정
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    SetScale(vResolution);

    // 배경은 화면 중앙에 고정
    SetPos(Vec2(vResolution.x / 2.f, vResolution.y / 2.f));
}

CBackground::~CBackground()
{
    // m_pBackgroundTex는 리소스 매니저에서 관리하므로 delete 하지 않음
}

void CBackground::Update()
{
    // 스크롤 가능한 배경인 경우 스크롤 오프셋 업데이트
    if (m_bScrollable && m_vScrollSpeed.x != 0.f)
    {
        float fDT = CTimeMgr::GetInst()->GetfDT();
        m_fScrollOffset += m_vScrollSpeed.x * fDT;

        // 텍스처 크기 기준으로 오프셋 순환
        if (m_pBackgroundTex)
        {
            UINT texWidth = m_pBackgroundTex->GetWidth();
            if (m_fScrollOffset >= (float)texWidth)
            {
                m_fScrollOffset -= (float)texWidth;
            }
            else if (m_fScrollOffset < 0.f)
            {
                m_fScrollOffset += (float)texWidth;
            }
        }
    }
}

void CBackground::Render(HDC _dc)
{
    if (nullptr == m_pBackgroundTex)
        return;

    // 배경 타입에 따라 다른 렌더링 방식 사용
    switch (m_eBackgroundType)
    {
    case BACKGROUND_TYPE::STATIC:
        RenderStaticBackground(_dc);
        break;
    case BACKGROUND_TYPE::SCROLLABLE:
        RenderScrollableBackground(_dc);
        break;
    case BACKGROUND_TYPE::PARALLAX:
        RenderParallaxBackground(_dc);
        break;
    default:
        RenderStaticBackground(_dc);
        break;
    }
}

void CBackground::SetupBackground(BACKGROUND_TYPE _eType)
{
    m_eBackgroundType = _eType;

    // 배경 타입별 기본 설정
    switch (_eType)
    {
    case BACKGROUND_TYPE::STATIC:
        m_bScrollable = false;
        m_vScrollSpeed = Vec2(0.f, 0.f);
        break;
    case BACKGROUND_TYPE::SCROLLABLE:
        m_bScrollable = true;
        m_vScrollSpeed = Vec2(50.f, 0.f);  // 기본 스크롤 속도
        break;
    case BACKGROUND_TYPE::PARALLAX:
        m_bScrollable = true;
        m_vScrollSpeed = Vec2(25.f, 0.f);  // 패럴랙스 효과용 느린 속도
        break;
    }
}

void CBackground::RenderStaticBackground(HDC _dc)
{
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    UINT texWidth = m_pBackgroundTex->GetWidth();
    UINT texHeight = m_pBackgroundTex->GetHeight();

    // 화면 전체에 배경 텍스처 스트레치
    StretchBlt(_dc,
        0, 0,
        (int)vResolution.x, (int)vResolution.y,
        m_pBackgroundTex->GetDC(),
        0, 0, texWidth, texHeight,
        SRCCOPY);
}

void CBackground::RenderScrollableBackground(HDC _dc)
{
    if (!m_bScrollable)
    {
        RenderStaticBackground(_dc);
        return;
    }

    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    UINT texWidth = m_pBackgroundTex->GetWidth();
    UINT texHeight = m_pBackgroundTex->GetHeight();

    // 카메라 위치에 따른 배경 스크롤 계산
    float fScrollRatio = 0.5f;  // 배경 스크롤 비율 (카메라 이동의 50%)
    int iOffsetX = (int)(vCameraPos.x * fScrollRatio);

    // 스크롤된 배경 렌더링 (루프)
    int iStartX = -(iOffsetX % (int)texWidth);

    for (int x = iStartX; x < (int)vResolution.x; x += texWidth)
    {
        StretchBlt(_dc,
            x, 0,
            texWidth, (int)vResolution.y,
            m_pBackgroundTex->GetDC(),
            0, 0, texWidth, texHeight,
            SRCCOPY);
    }
}

void CBackground::RenderParallaxBackground(HDC _dc)
{
    // 패럴랙스는 스크롤러블과 유사하지만 더 느린 스크롤 비율 사용
    if (!m_bScrollable)
    {
        RenderStaticBackground(_dc);
        return;
    }

    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    UINT texWidth = m_pBackgroundTex->GetWidth();
    UINT texHeight = m_pBackgroundTex->GetHeight();

    // 패럴랙스 효과를 위한 더 느린 스크롤 비율
    float fScrollRatio = 0.2f;  // 카메라 이동의 20%
    int iOffsetX = (int)(vCameraPos.x * fScrollRatio);

    // 패럴랙스 배경 렌더링
    int iStartX = -(iOffsetX % (int)texWidth);

    for (int x = iStartX; x < (int)vResolution.x; x += texWidth)
    {
        StretchBlt(_dc,
            x, 0,
            texWidth, (int)vResolution.y,
            m_pBackgroundTex->GetDC(),
            0, 0, texWidth, texHeight,
            SRCCOPY);
    }
}