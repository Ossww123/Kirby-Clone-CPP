#include "gamePCH.h"
#include "CUIGameHUD.h"
#include "CTexture.h"
#include "CResMgr.h"
#include "CBoss.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CTimeMgr.h"
#include "CSoundMgr.h"
#include "CKirby.h"
#include "CKirbyHealthSystem.h"

// ==== 고정 상수 매핑(소스 좌표/사이즈/배치) ====
const Vec2 CUIGameHUD::SRC_KIRBY_LIFE = Vec2(0.f, 0.f);
const Vec2 CUIGameHUD::SRC_NUM0 = Vec2(32.f, 0.f);
const Vec2 CUIGameHUD::SRC_HP_FULL = Vec2(112.f, 0.f);
const Vec2 CUIGameHUD::SRC_HP_EMPTY = Vec2(120.f, 0.f);
const Vec2 CUIGameHUD::SRC_BOSS_BAR = Vec2(128.f, 0.f);
const Vec2 CUIGameHUD::SRC_BOSS_FILL = Vec2(216.f, 8.f);

const Vec2 CUIGameHUD::SZ_KIRBY_LIFE = Vec2(24.f, 16.f);
const Vec2 CUIGameHUD::SZ_NUM = Vec2(8.f, 16.f);
const Vec2 CUIGameHUD::SZ_HP = Vec2(8.f, 16.f);
const Vec2 CUIGameHUD::SZ_BOSS_BAR = Vec2(80.f, 16.f);
const Vec2 CUIGameHUD::SZ_BOSS_FILL = Vec2(64.f, 5.f); // 원본 4 → 가독성 위해 5px

const Vec2 CUIGameHUD::POS_LIFE_BASE = Vec2(272.f, 588.f);
const Vec2 CUIGameHUD::OFF_LIFE_NUM = Vec2(64.f, 0.f);
const Vec2 CUIGameHUD::OFF_HEALTH = Vec2(160.f, 0.f);
const Vec2 CUIGameHUD::OFF_BOSS = Vec2(528.f, 0.f);

CUIGameHUD::CUIGameHUD() {}
CUIGameHUD::~CUIGameHUD() {}

void CUIGameHUD::Init()
{
    m_texUI = CResMgr::GetInst()->LoadTexture(L"UI", L"texture\\UI\\UI.bmp");
    if (!m_texUI) {
        MessageBox(nullptr, L"UI.bmp 로드 실패", L"UI", MB_OK);
    }
    m_showBossHP = false;
    m_fillT = 0.f;
    m_fillActive = false;

    // 화면 기준 위치 지정(원하면 여기서 고정)
    SetPos(Vec2(0.f, 0.f)); // base = 화면 좌상단
}

void CUIGameHUD::Update()
{
    if (m_fillActive) {
        const float dur = 1.5f; // 1.5초 채우기
        m_fillT += CTimeMgr::GetInst()->GetfDT() / dur;
        if (m_fillT >= 1.f) { m_fillT = 1.f; m_fillActive = false; }
    }
}

void CUIGameHUD::RenderUI(HDC dc, const Vec2& base)
{
    if (!m_texUI) return;

    // 플레이어/보스 조회
    CKirby* pl = FindPlayer();
    CBoss* bs = FindBoss();

    // 생명 & 플레이어 HP
    if (pl) {
        RenderKirbyLifeUI(dc, base + POS_LIFE_BASE);
        if (auto* hs = pl->GetHealth()) {                 // ← 일관되게 GetHealth() 사용
            RenderKirbyHealthUI(dc, base + POS_LIFE_BASE + OFF_HEALTH);
        }
    }

    // 보스 HP
    if (bs) {
        RenderBossHealthUI(dc, base + POS_LIFE_BASE + OFF_BOSS);
    }
}

void CUIGameHUD::StartBossHPFillAnimation()
{
    m_fillT = 0.f;
    m_fillActive = true;
    m_showBossHP = true;
    CSoundMgr::GetInst()->PlaySFX(L"boss_HP_fill");
}

// ========== 내부 렌더링 ==========
void CUIGameHUD::RenderKirbyLifeUI(HDC dc, const Vec2& base)
{
    // Kirby 머리 아이콘
    Blit(dc, SRC_KIRBY_LIFE, SZ_KIRBY_LIFE,
        base, SZ_KIRBY_LIFE * (float)UI_SCALE);

    // 생명 수 2자리
    if (CKirby* pl = FindPlayer()) {
        DrawNumber2(dc, pl->GetLives(), base + OFF_LIFE_NUM);
    }
    else {
        DrawNumber2(dc, 0, base + OFF_LIFE_NUM);
    }
}

void CUIGameHUD::RenderKirbyHealthUI(HDC dc, const Vec2& base)
{
    CKirby* pl = FindPlayer();
    if (!pl) return;

    auto* hs = pl->GetHealth();
    if (!hs) return;

    const int cur = hs->GetHP();       // GetCurrentHP → GetHP 이름만 다르면 맞춰서 사용
    const int max = hs->GetMaxHP();

    for (int i = 0; i < max; ++i) {
        const bool full = (i + 1) <= cur;
        DrawHealthCell(dc, full, base + Vec2(i * SZ_HP.x * UI_SCALE, 0.f));
    }
}

void CUIGameHUD::RenderBossHealthUI(HDC dc, const Vec2& base)
{
    if (!m_showBossHP) return;

    CBoss* boss = FindBoss();
    if (!boss || boss->IsDefeated()) return;

    // 바탕 바
    Blit(dc, SRC_BOSS_BAR, SZ_BOSS_BAR,
        base, SZ_BOSS_BAR * (float)UI_SCALE);

    const float hpRatio = boss->GetHPRatio();
    const float disp = m_fillActive ? (hpRatio * m_fillT) : hpRatio;
    if (disp <= 0.f) return;

    Vec2 srcFill = SZ_BOSS_FILL;
    srcFill.x *= disp;
    Vec2 dstFill = srcFill * (float)UI_SCALE;

    // 왼쪽 고정으로 채우기
    Vec2 fillPos = base + Vec2(0.f, 2.5f * UI_SCALE);
    float diff = (SZ_BOSS_FILL.x - srcFill.x) * UI_SCALE / 2.f;
    fillPos.x -= diff;

    Blit(dc, SRC_BOSS_FILL, srcFill, fillPos, dstFill);
}

// ========== 도우미 ==========
void CUIGameHUD::Blit(HDC dc, Vec2 sPos, Vec2 sSize, Vec2 dPos, Vec2 dSize)
{
    if (!m_texUI) return;
    m_texUI->RenderSpriteWithColorKey(dc, dPos, sPos, sSize, dSize, RGB(255, 0, 255));
}

void CUIGameHUD::DrawNumber2(HDC dc, int v, Vec2 pos)
{
    v = std::clamp(v, 0, 99);
    const int tens = v / 10;
    const int ones = v % 10;

    Vec2 srcT = SRC_NUM0 + Vec2(SZ_NUM.x * tens, 0.f);
    Vec2 srcO = SRC_NUM0 + Vec2(SZ_NUM.x * ones, 0.f);

    Blit(dc, srcT, SZ_NUM, pos, SZ_NUM * (float)UI_SCALE);
    Blit(dc, srcO, SZ_NUM, pos + Vec2(SZ_NUM.x * UI_SCALE, 0.f), SZ_NUM * (float)UI_SCALE);
}

void CUIGameHUD::DrawHealthCell(HDC dc, bool full, Vec2 pos)
{
    Blit(dc, full ? SRC_HP_FULL : SRC_HP_EMPTY, SZ_HP, pos, SZ_HP * (float)UI_SCALE);
}

// ========== 데이터 수집 ==========
CKirby* CUIGameHUD::FindPlayer() const
{
    CScene* sc = CSceneMgr::GetInst()->GetCurScene();
    if (!sc) return nullptr;
    const auto& v = sc->GetGroupObject(GROUP_TYPE::PLAYER);
    for (auto* o : v) if (auto* k = dynamic_cast<CKirby*>(o)) return k;
    return nullptr;
}

CBoss* CUIGameHUD::FindBoss() const
{
    CScene* sc = CSceneMgr::GetInst()->GetCurScene();
    if (!sc) return nullptr;
    const auto& v = sc->GetGroupObject(GROUP_TYPE::MONSTER);
    for (auto* o : v) {
        if (auto* b = dynamic_cast<CBoss*>(o)) return b;
    }
    return nullptr;
}
