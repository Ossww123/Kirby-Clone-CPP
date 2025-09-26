#pragma once
#include "CUIElement.h"

class CTexture;
class CBoss;
class CKirby;
class CKirbyHealthSystem;

class CUIGameHUD : public CUIElement
{
public:
    CUIGameHUD();
    ~CUIGameHUD() override;

    // 초기화(텍스처 로드 등)
    void Init();
    void Update() override;     // 보스 HP 채우기 연출만 갱신
    // Render는 CUIElement가 호출 → RenderUI를 구현
    void RenderUI(HDC dc, const Vec2& screenPos) override;

    // 외부 제어 (보스전 연출)
    void ShowBossHP(bool show) { m_showBossHP = show; }
    void StartBossHPFillAnimation();

private:
    // 내부 렌더링(기존 CUIMgr에서 나뉘던 함수)
    void RenderKirbyLifeUI(HDC dc, const Vec2& base);
    void RenderKirbyHealthUI(HDC dc, const Vec2& base);
    void RenderBossHealthUI(HDC dc, const Vec2& base);

    // 스프라이트 도우미
    void Blit(HDC dc, Vec2 srcPos, Vec2 srcSize, Vec2 dstPos, Vec2 dstSize);
    void DrawNumber2(HDC dc, int value, Vec2 pos);     // 2자리(01~99)
    void DrawHealthCell(HDC dc, bool full, Vec2 pos);

    // 데이터 수집
    CKirby* FindPlayer() const;
    CBoss* FindBoss()   const;

private:
    // === 리소스 ===
    CTexture* m_texUI{ nullptr };

    // === 보스 HP 연출 ===
    bool  m_showBossHP{ false };
    float m_fillT{ 0.f };            // 0→1 채우기
    bool  m_fillActive{ false };

    // === 상수(기존 좌표/크기 그대로) ===
    static const Vec2 SRC_KIRBY_LIFE;    // 24x16 @ (0,0)
    static const Vec2 SRC_NUM0;          // 8x16  @ (32,0) → +8*i
    static const Vec2 SRC_HP_FULL;       // 8x16  @ (112,0)
    static const Vec2 SRC_HP_EMPTY;      // 8x16  @ (120,0)
    static const Vec2 SRC_BOSS_BAR;      // 80x16 @ (128,0)
    static const Vec2 SRC_BOSS_FILL;     // 64x4  @ (216,8)

    static const Vec2 SZ_KIRBY_LIFE;     // 24x16
    static const Vec2 SZ_NUM;            // 8x16
    static const Vec2 SZ_HP;             // 8x16
    static const Vec2 SZ_BOSS_BAR;       // 80x16
    static const Vec2 SZ_BOSS_FILL;      // 64x4(원본은 64x4, 약간 두껍게 보이고 싶다면 64x5)

    static const int  UI_SCALE = 4;

    // === 화면 내 배치(기존과 동일, base는 화면 좌표계 기준) ===
    static const Vec2 POS_LIFE_BASE;     // (272,588)
    static const Vec2 OFF_LIFE_NUM;      // + (64,0)
    static const Vec2 OFF_HEALTH;        // + (160,0)
    static const Vec2 OFF_BOSS;          // + (528,0)
};
