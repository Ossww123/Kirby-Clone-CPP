#pragma once
#include "pch.h"

class CTexture;
class CPlayer;
class CBoss;

class CUIMgr
{
    SINGLE(CUIMgr);

private:
    CTexture* m_pUITexture;      // UI.bmp 텍스처

    // === UI 스프라이트 좌표 상수 ===
    static const Vec2 KIRBY_LIFE_ICON_POS;    // 커비 생명 아이콘 (0,0) 24x16
    static const Vec2 NUMBER_0_POS;           // 숫자 0 (32,0) 8x16
    static const Vec2 HEALTH_FULL_POS;        // 체력칸 (112,0) 8x16  
    static const Vec2 HEALTH_EMPTY_POS;       // 빈 체력칸 (120,0) 8x16
    static const Vec2 BOSS_HP_BAR_POS;        // 보스 체력바 (128,0) 80x16
    static const Vec2 BOSS_HP_FILL_POS;       // 보스 체력바 내부 (216,8) 64x4

    // === 화면 렌더링 위치 (4배 스케일) ===
    static const Vec2 KIRBY_LIFE_SCREEN_POS;  // 커비 생명 UI (224,588)
    static const Vec2 LIFE_COUNT_OFFSET;      // 생명수 숫자 오프셋 (96,0)
    static const Vec2 HEALTH_OFFSET;          // 체력 UI 오프셋 (128,0) - 32px 띄움
    static const Vec2 BOSS_HP_OFFSET;         // 보스 HP 오프셋 (224,0) - 32px 띄움

    // === 스프라이트 크기 상수 ===
    static const Vec2 KIRBY_LIFE_ICON_SIZE;   // 24x16
    static const Vec2 NUMBER_SIZE;            // 8x16
    static const Vec2 HEALTH_SIZE;            // 8x16
    static const Vec2 BOSS_HP_BAR_SIZE;       // 80x16
    static const Vec2 BOSS_HP_FILL_SIZE;      // 64x4

    static const int UI_SCALE;               // UI 스케일 (4배)

    // === 보스 HP 바 상태 ===
    bool m_bShowBossHP;                      // 보스 HP 바 표시 여부
    float m_fBossHPFillAnimation;            // HP 바 채우기 애니메이션 진행도 (0.0 ~ 1.0)
    bool m_bBossHPFillActive;                // HP 바 채우기 애니메이션 활성 상태

public:
    void Init();                            // UI.bmp 로드
    void Update();                          // HP 바 애니메이션 업데이트
    void RenderGameUI(HDC _dc);             // 게임 플레이 UI 통합 렌더링

    // === 보스 HP 바 제어 ===
    void ShowBossHP(bool _bShow) { m_bShowBossHP = _bShow; }
    void StartBossHPFillAnimation();         // HP 바 채우기 애니메이션 시작
    bool IsBossHPFillComplete() const { return m_fBossHPFillAnimation >= 1.f; }

private:
    // === UI 요소별 렌더링 ===
    void RenderKirbyLifeUI(HDC _dc, int _iLives);
    void RenderKirbyHealthUI(HDC _dc, int _iCurrentHP, int _iMaxHP);
    void RenderBossHealthUI(HDC _dc, CBoss* _pBoss);

    // === 스프라이트별 렌더링 헬퍼 ===
    void RenderNumber(HDC _dc, int _iNumber, Vec2 _vPos);
    void RenderHealthBar(HDC _dc, int _iCurrent, int _iMax, Vec2 _vPos, bool _bIsFull);
    void RenderSprite(HDC _dc, Vec2 _vSrcPos, Vec2 _vSrcSize, Vec2 _vDestPos, Vec2 _vDestSize);

    // === 데이터 수집 헬퍼 ===
    CPlayer* FindPlayer();
    CBoss* FindBoss();
};