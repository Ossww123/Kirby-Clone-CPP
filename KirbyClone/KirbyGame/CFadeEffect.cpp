#include "pch.h"
#include "CFadeEffect.h"
#include "CTimeMgr.h"
#include "CCore.h"
#include "CEventMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CPlayerStateMachine.h"

CFadeEffect::CFadeEffect()
    : m_bActive(false)
    , m_eFadeType(FADE_TYPE::FADE_OUT)
    , m_eFadeColor(FADE_COLOR::BLACK)
    , m_fFadeTimer(0.f)
    , m_fFadeDuration(1.f)
    , m_iAlpha(0)
    , m_iMaxAlpha(255)
    , m_dwCallbackData(0)
    , m_bHoldFade(false)
    , m_bAbilityPresentation(false)
    , m_eAbilityPhase(ABILITY_PRESENTATION_PHASE::NONE)
    , m_eAcquiredAbility(COPY_ABILITY::NONE)
    , m_fAbilityTimer(0.f)
    , m_fAbilityShowDuration(2.0f)
{
}

CFadeEffect::~CFadeEffect()
{
}

void CFadeEffect::Init()
{
    m_bActive = false;
    m_fFadeTimer = 0.f;
    m_iAlpha = 0;
    m_dwCallbackData = 0;
    m_bHoldFade = false;
    
    // 카피능력 연출 초기화
    m_bAbilityPresentation = false;
    m_eAbilityPhase = ABILITY_PRESENTATION_PHASE::NONE;
    m_fAbilityTimer = 0.f;
}

void CFadeEffect::Update()
{
    if (!m_bActive && !m_bAbilityPresentation)
        return;

    if (m_bActive)
        UpdateFade();
}

void CFadeEffect::UpdateFade()
{
    // HoldFade가 활성화된 상태에서는 타이머 업데이트 중단 (단, 콜백 데이터가 3이면 예외)
    if (m_bHoldFade && m_dwCallbackData != 3)
    {
        return;
    }
    
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fFadeTimer += fDT;

    // 진행률 계산 (0.0 ~ 1.0)
    float fProgress = m_fFadeTimer / m_fFadeDuration;
    if (fProgress > 1.f)
        fProgress = 1.f;

    // 페이드 타입에 따라 알파값 계산
    if (m_eFadeType == FADE_TYPE::FADE_OUT)
    {
        // 투명 → 불투명 (0 → m_iMaxAlpha)
        m_iAlpha = (int)((float)m_iMaxAlpha * fProgress);
    }
    else // FADE_IN
    {
        // 불투명 → 투명 (m_iMaxAlpha → 0)
        m_iAlpha = (int)((float)m_iMaxAlpha * (1.f - fProgress));
    }

    // 페이드 완료 체크
    if (fProgress >= 1.f)
    {
        OnFadeComplete();
    }
}

void CFadeEffect::Render(HDC _dc)
{
    // 카피능력 연출 중 SHOW_ABILITY 단계에서는 항상 완전 암전 유지
    bool bShowAbilityPhase = (m_bAbilityPresentation && m_eAbilityPhase == ABILITY_PRESENTATION_PHASE::SHOW_ABILITY);
    
    // 일반 페이드 효과 렌더링 또는 연출 중 암전 렌더링 또는 HoldFade 상태 렌더링
    if ((m_bActive && m_iAlpha > 0) || bShowAbilityPhase || (m_bHoldFade && m_iAlpha > 0))
    {
        // 화면 크기 가져오기
        Vec2 vResolution = CCore::GetInst()->GetResolution();
        RECT screenRect = { 0, 0, (int)vResolution.x, (int)vResolution.y };

        // 브러시 생성
        HBRUSH hBrush;
        if (m_eFadeColor == FADE_COLOR::WHITE)
        {
            hBrush = CreateSolidBrush(RGB(255, 255, 255));
        }
        else // BLACK
        {
            hBrush = CreateSolidBrush(RGB(0, 0, 0));
        }

        // 블렌드 함수를 위한 설정
        BLENDFUNCTION blendFunc = {};
        blendFunc.BlendOp = AC_SRC_OVER;
        blendFunc.BlendFlags = 0;
        // SHOW_ABILITY 단계에서는 10% 암전 (알파 25), 그 외에는 기존 알파값 사용
        blendFunc.SourceConstantAlpha = bShowAbilityPhase ? 25 : m_iAlpha;
        blendFunc.AlphaFormat = 0;

        // 메모리 DC 생성 및 페이드 렌더링
        HDC hMemDC = CreateCompatibleDC(_dc);
        HBITMAP hBitmap = CreateCompatibleBitmap(_dc, (int)vResolution.x, (int)vResolution.y);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

        // 페이드 색상으로 메모리 DC 채우기
        FillRect(hMemDC, &screenRect, hBrush);

        // 알파 블렌딩으로 화면에 렌더링
        AlphaBlend(_dc, 0, 0, (int)vResolution.x, (int)vResolution.y,
                   hMemDC, 0, 0, (int)vResolution.x, (int)vResolution.y, blendFunc);

        // 리소스 정리
        SelectObject(hMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        DeleteObject(hBrush);
    }
}

void CFadeEffect::StartFadeOut(FADE_COLOR _eColor, float _fDuration, DWORD_PTR _dwCallbackData)
{
    m_bActive = true;
    m_eFadeType = FADE_TYPE::FADE_OUT;
    m_eFadeColor = _eColor;
    m_fFadeDuration = _fDuration;
    m_fFadeTimer = 0.f;
    m_iAlpha = 0;
    m_iMaxAlpha = 255; // 기본값
    m_dwCallbackData = _dwCallbackData;
}

void CFadeEffect::StartFadeOut(FADE_COLOR _eColor, float _fDuration, int _iMaxAlpha, DWORD_PTR _dwCallbackData)
{
    m_bActive = true;
    m_eFadeType = FADE_TYPE::FADE_OUT;
    m_eFadeColor = _eColor;
    m_fFadeDuration = _fDuration;
    m_fFadeTimer = 0.f;
    m_iAlpha = 0;
    m_iMaxAlpha = _iMaxAlpha; // 사용자 지정 최대 알파값
    m_dwCallbackData = _dwCallbackData;
}

void CFadeEffect::StartFadeIn(FADE_COLOR _eColor, float _fDuration, DWORD_PTR _dwCallbackData)
{
    m_bActive = true;
    m_eFadeType = FADE_TYPE::FADE_IN;
    m_eFadeColor = _eColor;
    m_fFadeDuration = _fDuration;
    m_fFadeTimer = 0.f;
    m_iAlpha = m_iMaxAlpha; // 이전 FadeOut에서 사용한 MaxAlpha 사용
    m_dwCallbackData = _dwCallbackData;
}

void CFadeEffect::ForceComplete()
{
    if (!m_bActive)
        return;

    m_fFadeTimer = m_fFadeDuration;
    
    if (m_eFadeType == FADE_TYPE::FADE_OUT)
        m_iAlpha = 255;
    else
        m_iAlpha = 0;
        
    OnFadeComplete();
}

void CFadeEffect::OnFadeComplete()
{
    // HoldFade가 활성화된 경우에도 콜백 데이터가 3 (문 입장)이면 완료 처리
    if (m_bHoldFade && m_dwCallbackData != 3)
    {
        // 페이드는 활성 상태로 유지하고 타이머만 고정
        return;
    }
    
    m_bActive = false;
    
    // FADE_COMPLETE 이벤트 발생
    tEvent fadeCompleteEvent = {};
    fadeCompleteEvent.eType = EVENT_TYPE::FADE_COMPLETE;
    fadeCompleteEvent.wParam = m_dwCallbackData;  // 콜백 데이터 전달
    CEventMgr::GetInst()->AddEvent(fadeCompleteEvent);
    
    m_dwCallbackData = 0;  // 콜백 데이터 초기화
}

// === 카피능력 연출 관련 구현 ===

// 능력 연출 관련 함수는 제거됨 - 단순한 페이드 효과만 사용

// 복잡한 능력 연출 코드 제거됨 - 단순한 페이드 효과만 사용

// === 암전 유지 관련 구현 ===

void CFadeEffect::HoldCurrentFade()
{
    m_bHoldFade = true;
}

void CFadeEffect::ReleaseFadeHold()
{
    m_bHoldFade = false;
}