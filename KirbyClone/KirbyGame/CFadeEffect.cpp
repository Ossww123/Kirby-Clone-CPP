#include "gamePCH.h"
#include "CFadeEffect.h"

#include "CTimeMgr.h"
#include "CCore.h"
#include "CEventMgr.h"

CFadeEffect::CFadeEffect() = default;
CFadeEffect::~CFadeEffect() = default;

void CFadeEffect::Init()
{
    m_active = false;
    m_timer = 0.f;
    m_progress = 0.f;
    m_alpha = 0;
    m_token = 0;
    m_maxAlpha = 255;
    m_type = FADE_TYPE::FadeOut;
    m_color = FADE_COLOR::BLACK;
    m_duration = 1.f;
}

void CFadeEffect::Begin(FADE_TYPE type, FADE_COLOR color, float duration, uintptr_t token, int maxAlpha)
{
    m_active = true;
    m_type = type;
    m_color = color;
    m_duration = (std::max)(0.0001f, duration);
    m_timer = 0.f;
    m_progress = 0.f;
    m_maxAlpha = std::clamp(maxAlpha, 0, 255);
    m_alpha = (type == FADE_TYPE::FadeIn) ? m_maxAlpha : 0; // IN은 불투명→투명 시작
    m_token = token;
}

void CFadeEffect::StartFadeOut(FADE_COLOR color, float duration, uintptr_t token, int maxAlpha)
{
    Begin(FADE_TYPE::FadeOut, color, duration, token, maxAlpha);
}

void CFadeEffect::StartFadeIn(FADE_COLOR color, float duration, uintptr_t token, int maxAlpha)
{
    Begin(FADE_TYPE::FadeIn, color, duration, token, maxAlpha);
}

void CFadeEffect::Update()
{
    if (!m_active) return;

    m_timer += CTimeMgr::GetInst()->GetfDT();
    m_progress = (std::min)(1.f, m_timer / m_duration);

    // 0~1 보간
    if (m_type == FADE_TYPE::FadeOut) {
        // 투명(0) → 불투명(max)
        m_alpha = (int)(m_maxAlpha * m_progress);
    }
    else {
        // 불투명(max) → 투명(0)
        m_alpha = (int)(m_maxAlpha * (1.f - m_progress));
    }

    if (m_progress >= 1.f) {
        Finish();
    }
}

void CFadeEffect::Render(HDC dc)
{
    if (!m_active && m_alpha <= 0) return;
    if (m_alpha <= 0) return;

    const Vec2 res = CCore::GetInst()->GetResolution();
    RECT rc = { 0, 0, (int)res.x, (int)res.y };

    // 색 브러시
    const COLORREF col = (m_color == FADE_COLOR::WHITE) ? RGB(255, 255, 255) : RGB(0, 0, 0);
    HBRUSH hBrush = CreateSolidBrush(col);

    // 오프스크린 DC
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

    FillRect(memDC, &rc, hBrush);

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = (BYTE)m_alpha; // 0~255
    bf.AlphaFormat = 0;

    AlphaBlend(dc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, rc.right, rc.bottom, bf);

    // 정리
    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
    DeleteObject(hBrush);
}

void CFadeEffect::ForceComplete()
{
    if (!m_active) return;

    // 최종 알파 상태로 설정
    m_progress = 1.f;
    m_alpha = (m_type == FADE_TYPE::FadeOut) ? m_maxAlpha : 0;

    Finish();
}

void CFadeEffect::Finish()
{
    m_active = false;

    // 단순히 "끝남"을 알림(토큰 그대로 전달)
    tEvent e(EVENT_TYPE::FADE_COMPLETE, m_token, 0);
    CEventMgr::GetInst()->AddEvent(e);

    m_token = 0;
}
