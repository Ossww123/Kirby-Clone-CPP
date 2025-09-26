#pragma once
#include "gamePCH.h"
#include "EventDef.h"   // tEvent, EVENT_TYPE

enum class FADE_TYPE { FadeIn, FadeOut };
enum class FADE_COLOR { BLACK, WHITE };

// 화면 페이드 전용: 색/시간/알파만 다룸
class CFadeEffect
{
    SINGLE(CFadeEffect);

public:
    void Init();                      // 상태 초기화
    void Update();                    // 타이머/알파 갱신
    void Render(HDC dc);              // 전체 화면에 알파 블렌딩

    // 시작 API (편의 오버로드)
    void StartFadeOut(FADE_COLOR color, float duration, uintptr_t token = 0, int maxAlpha = 255);
    void StartFadeIn(FADE_COLOR color, float duration, uintptr_t token = 0, int maxAlpha = 255);

    // 상태
    bool IsActive()   const { return m_active; }
    bool IsComplete() const { return m_progress >= 1.f; } // 참고용

    // 강제 완료 (즉시 완료 프레임에 FADE_COMPLETE 발행)
    void ForceComplete();

private:
    void Begin(FADE_TYPE type, FADE_COLOR color, float duration, uintptr_t token, int maxAlpha);
    void Finish();  // FADE_COMPLETE 이벤트 발행

private:
    // 파라미터/상태
    bool       m_active = false;
    FADE_TYPE  m_type = FADE_TYPE::FadeOut;
    FADE_COLOR m_color = FADE_COLOR::BLACK;
    float      m_duration = 1.f;   // 초
    float      m_timer = 0.f;   // 누적
    float      m_progress = 0.f;   // 0~1
    int        m_alpha = 0;     // 0~255
    int        m_maxAlpha = 255;

    // 완료 통지용 토큰(그저 전달만 함)
    uintptr_t  m_token = 0;
};
