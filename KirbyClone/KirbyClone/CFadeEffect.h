#pragma once
#include "pch.h"

enum class COPY_ABILITY;

enum class FADE_TYPE
{
    FADE_IN,      // 검은색/흰색에서 투명하게
    FADE_OUT,     // 투명에서 검은색/흰색으로
};

enum class FADE_COLOR
{
    BLACK,
    WHITE,
};

// 카피능력 연출 단계
enum class ABILITY_PRESENTATION_PHASE
{
    NONE,           // 연출 없음
    FADE_OUT,       // 어두워지기
    SHOW_ABILITY,   // 능력 시연
    FADE_IN,        // 밝아지기
    COMPLETE        // 완료
};

class CFadeEffect
{
    SINGLE(CFadeEffect);

private:
    bool        m_bActive;          // 페이드 효과 활성화 여부
    FADE_TYPE   m_eFadeType;        // 페이드 타입 (IN/OUT)
    FADE_COLOR  m_eFadeColor;       // 페이드 색상 (검은색/흰색)
    float       m_fFadeTimer;       // 페이드 진행 시간
    float       m_fFadeDuration;    // 페이드 총 지속 시간
    int         m_iAlpha;           // 현재 알파값 (0~255)
    int         m_iMaxAlpha;        // 최대 알파값 (기본 255, 능력 연출시 25)
    
    DWORD_PTR   m_dwCallbackData;   // 콜백에 전달할 데이터 (선택적)
    bool        m_bHoldFade;        // 페이드 상태 유지 플래그
    
    // === 카피능력 연출 관련 ===
    bool                        m_bAbilityPresentation;    // 카피능력 연출 진행 중
    ABILITY_PRESENTATION_PHASE  m_eAbilityPhase;           // 연출 단계
    COPY_ABILITY                m_eAcquiredAbility;        // 획득한 능력
    float                       m_fAbilityTimer;           // 연출 타이머
    float                       m_fAbilityShowDuration;    // 능력 시연 지속시간

public:
    void Init();
    void Update();
    void Render(HDC _dc);

public:
    // === 페이드 시작 함수 ===
    void StartFadeOut(FADE_COLOR _eColor, float _fDuration, DWORD_PTR _dwCallbackData = 0);
    void StartFadeOut(FADE_COLOR _eColor, float _fDuration, int _iMaxAlpha, DWORD_PTR _dwCallbackData = 0);
    void StartFadeIn(FADE_COLOR _eColor, float _fDuration, DWORD_PTR _dwCallbackData = 0);
    
    // === 상태 확인 ===
    bool IsActive() const { return m_bActive; }
    bool IsComplete() const { return m_fFadeTimer >= m_fFadeDuration; }
    bool IsAbilityPresentationActive() const { return m_bAbilityPresentation; }
    bool IsHoldingFade() const { return m_bHoldFade; }
    
    // === 즉시 완료 ===
    void ForceComplete();
    
    // === 암전 유지 제어 ===
    void HoldCurrentFade();     // 현재 페이드 상태 유지
    void ReleaseFadeHold();     // 암전 유지 해제
    
    // === 카피능력 연출 ===
    bool IsPausingGame() const { return m_bAbilityPresentation; }
    
private:
    void UpdateFade();
    void OnFadeComplete();
};