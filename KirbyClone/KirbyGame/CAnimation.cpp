#include "gamePCH.h"
#include "CAnimation.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CTimeMgr.h"
#pragma comment(lib, "msimg32.lib")

CAnimation::CAnimation() {}
CAnimation::~CAnimation() { ReleaseFlipSurfaces(); }

void CAnimation::Update()
{
    if (m_vecFrame.empty()) return;

    m_fAccTime += CTimeMgr::GetInst()->GetfDT();
    const float curDur = m_vecFrame[m_iCurFrame].fDuration;

    if (m_fAccTime >= curDur)
    {
        m_fAccTime -= curDur;
        ++m_iCurFrame;
        if (m_iCurFrame >= (int)m_vecFrame.size())
        {
            if (m_bLoop) m_iCurFrame = 0;
            else {
                m_iCurFrame = (int)m_vecFrame.size() - 1;
                m_bFinish = true;
            }
        }
    }
}

void CAnimation::Reset()
{
    m_iCurFrame = 0;
    m_fAccTime = 0.f;
    m_bFinish = false;
}

void CAnimation::AddFrame(Vec2 vLT, Vec2 vSlice, float dur, Vec2 vOffset)
{
    if (dur <= 0.f) return;
    tAnimFrame fr;
    fr.vLT = vLT; fr.vSlice = vSlice; fr.vOffset = vOffset; fr.fDuration = dur;
    m_vecFrame.push_back(fr);
}

void CAnimation::ClearFrames()
{
    m_vecFrame.clear();
    Reset();
}

void CAnimation::RenderScaled(HDC _dc, const Vec2& _vWorldPos, float _fScale, bool _flipX)
{
    if (!m_pSheet || m_vecFrame.empty()) return;

    // 카메라 좌표 변환
    Vec2 vScreen = CCamera::GetInst()->GetRenderPos(_vWorldPos);

    const tAnimFrame& fr = m_vecFrame[m_iCurFrame];
    RenderFrame(_dc, fr, vScreen, _fScale, _flipX);
}

void CAnimation::RenderFrame(HDC _dc, const tAnimFrame& fr, const Vec2& vScreenPos, float fScale, bool flipX)
{
    if (!m_pSheet) return;

    const int srcL = (int)fr.vLT.x;
    const int srcT = (int)fr.vLT.y;
    const int srcW = (int)fr.vSlice.x;
    const int srcH = (int)fr.vSlice.y;

    const int dstW = (int)(fr.vSlice.x * fScale);
    const int dstH = (int)(fr.vSlice.y * fScale);

    // 피벗 오프셋 적용(픽셀 스냅)
    const int dstX = (int)(vScreenPos.x - fr.vOffset.x * fScale);
    const int dstY = (int)(vScreenPos.y - fr.vOffset.y * fScale);

    HDC  hSrcDC = m_pSheet->GetDC();
    UINT ck = m_pSheet->GetColorKey(); // CTexture에 반드시 구현

    if (!flipX)
    {
        TransparentBlt(_dc, dstX - dstW / 2, dstY - dstH / 2, dstW, dstH,
            hSrcDC, srcL, srcT, srcW, srcH, ck);
        return;
    }

    // === flipX ===
    EnsureFlipSurfaces(POINT{ dstW, dstH });

    // 1) 원본을 DC1에 스케일 붙여넣기
    TransparentBlt(m_hFlipDC1, 0, 0, dstW, dstH,
        hSrcDC, srcL, srcT, srcW, srcH, ck);

    // 2) DC1 → DC2로 가로 플립 복사(음수 폭)
    StretchBlt(m_hFlipDC2,
        dstW - 1, 0, -dstW, dstH,
        m_hFlipDC1, 0, 0, dstW, dstH, SRCCOPY);

    // 3) DC2를 화면으로 전송
    TransparentBlt(_dc, dstX - dstW / 2, dstY - dstH / 2, dstW, dstH,
        m_hFlipDC2, 0, 0, dstW, dstH, ck);
}

void CAnimation::EnsureFlipSurfaces(const POINT& sizePx)
{
    if (m_cachedSize.x == sizePx.x && m_cachedSize.y == sizePx.y &&
        m_hFlipDC1 && m_hFlipDC2 && m_hFlipBmp1 && m_hFlipBmp2)
        return;

    ReleaseFlipSurfaces();

    // 대상 DC 기준으로 호환 DC/Bitmap 생성
    HDC hCompat = GetDC(nullptr);
    m_hFlipDC1 = CreateCompatibleDC(hCompat);
    m_hFlipDC2 = CreateCompatibleDC(hCompat);
    m_hFlipBmp1 = CreateCompatibleBitmap(hCompat, sizePx.x, sizePx.y);
    m_hFlipBmp2 = CreateCompatibleBitmap(hCompat, sizePx.x, sizePx.y);
    ReleaseDC(nullptr, hCompat);

    SelectObject(m_hFlipDC1, m_hFlipBmp1);
    SelectObject(m_hFlipDC2, m_hFlipBmp2);

    // 마젠타로 초기화(투명색)
    RECT rc{ 0,0,sizePx.x,sizePx.y };
    HBRUSH magenta = CreateSolidBrush(RGB(255, 0, 255));
    FillRect(m_hFlipDC1, &rc, magenta);
    FillRect(m_hFlipDC2, &rc, magenta);
    DeleteObject(magenta);

    m_cachedSize = sizePx;
}

void CAnimation::ReleaseFlipSurfaces()
{
    if (m_hFlipDC1) { DeleteDC(m_hFlipDC1); m_hFlipDC1 = nullptr; }
    if (m_hFlipDC2) { DeleteDC(m_hFlipDC2); m_hFlipDC2 = nullptr; }
    if (m_hFlipBmp1) { DeleteObject(m_hFlipBmp1); m_hFlipBmp1 = nullptr; }
    if (m_hFlipBmp2) { DeleteObject(m_hFlipBmp2); m_hFlipBmp2 = nullptr; }
    m_cachedSize = POINT{ 0,0 };
}
