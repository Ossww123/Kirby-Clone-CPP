// CPlayerHealthSystem.cpp
#include "pch.h"
#include "CPlayerHealthSystem.h"
#include "CPlayer.h"
#include "CRigidBody.h"
#include "CTimeMgr.h"
#include "CCore.h"
#include "CCamera.h"
#include "CSceneMgr.h"

CPlayerHealthSystem::CPlayerHealthSystem(CPlayer* _pOwner)
    : m_pOwner(_pOwner)
    , m_iMaxHP(6)
    , m_iCurrentHP(6)
    , m_bIsInvincible(false)
    , m_fInvincibleTime(2.0f)
    , m_fInvincibleTimer(0.0f)
    , m_fBlinkInterval(0.1f)
    , m_bIsGameOver(false)
    , m_fGameOverY(1280.0f)
    , m_fGameOverTimer(0.0f)
    , m_fGameOverDelay(2.0f)
    , m_bKnockbackActive(false)
    , m_fKnockbackTimer(0.0f)
    , m_fKnockbackDuration(0.3f)
    , m_vKnockbackForce(Vec2(0.f, 0.f))
{
}

CPlayerHealthSystem::~CPlayerHealthSystem()
{
}

void CPlayerHealthSystem::Init()
{
    // 체력 완전 회복
    m_iCurrentHP = m_iMaxHP;
    m_bIsGameOver = false;
    m_bIsInvincible = false;
    m_fInvincibleTimer = 0.0f;
    m_fGameOverTimer = 0.0f;
    m_bKnockbackActive = false;
    m_fKnockbackTimer = 0.0f;
}

void CPlayerHealthSystem::Update()
{
    if (!m_pOwner)
        return;

    // 게임오버 상태 처리
    if (m_bIsGameOver)
    {
        UpdateGameOver();
        return;
    }

    // 일반 업데이트
    UpdateInvincible();
    UpdateKnockback();
    CheckGameOverConditions();
}

void CPlayerHealthSystem::Render(HDC _dc)
{
    // 체력 UI는 항상 렌더링
    RenderHealthUI(_dc);

    // 게임오버 UI
    if (m_bIsGameOver)
    {
        RenderGameOverUI(_dc);
    }
}

// === 체력 관리 함수들 ===

void CPlayerHealthSystem::TakeDamage(int _iDamage, Vec2 _vKnockbackDir)
{
    // 무적 상태거나 게임오버 상태면 데미지 무시
    if (m_bIsInvincible || m_bIsGameOver)
        return;

    // 체력 감소
    m_iCurrentHP -= _iDamage;

    // 체력이 0 이하가 되면 게임오버
    if (m_iCurrentHP <= 0)
    {
        m_iCurrentHP = 0;
        ForceGameOver();
        return;
    }

    // 피격 효과
    StartInvincible();
    PlayDamageEffects();

    // 넉백 적용
    if (_vKnockbackDir.Length() > 0.1f)
    {
        ApplyKnockback(_vKnockbackDir);
    }
}

void CPlayerHealthSystem::Heal(int _iHeal)
{
    if (m_bIsGameOver)
        return;

    m_iCurrentHP += _iHeal;

    // 최대 체력 제한
    if (m_iCurrentHP > m_iMaxHP)
        m_iCurrentHP = m_iMaxHP;

    PlayHealEffects();
}

void CPlayerHealthSystem::SetHP(int _iHP)
{
    m_iCurrentHP = _iHP;

    // 범위 제한
    if (m_iCurrentHP < 0)
        m_iCurrentHP = 0;
    if (m_iCurrentHP > m_iMaxHP)
        m_iCurrentHP = m_iMaxHP;

    // 체력이 0이면 게임오버
    if (m_iCurrentHP <= 0)
        ForceGameOver();
}

void CPlayerHealthSystem::SetMaxHP(int _iMaxHP)
{
    m_iMaxHP = _iMaxHP;

    // 현재 체력이 최대치를 넘으면 조정
    if (m_iCurrentHP > m_iMaxHP)
        m_iCurrentHP = m_iMaxHP;
}

// === 상태 체크 함수들 ===

bool CPlayerHealthSystem::ShouldRenderBlink() const
{
    if (!m_bIsInvincible)
        return false;

    // 깜빡임 패턴 계산
    int iBlinkCount = (int)(m_fInvincibleTimer / m_fBlinkInterval);
    return (iBlinkCount % 2 == 0);
}

void CPlayerHealthSystem::ForceGameOver()
{
    m_bIsGameOver = true;
    m_fGameOverTimer = 0.0f;
    PlayGameOverEffects();
}

void CPlayerHealthSystem::RestartStage()
{
    // 상태 초기화
    Init();

    // 플레이어 위치 및 상태 리셋
    if (m_pOwner)
    {
        // 시작 위치로 이동
        m_pOwner->SetPos(Vec2(640.f, 384.f));

        // 속도 초기화
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            pRigidBody->SetVelocity(Vec2(0.f, 0.f));
            pRigidBody->SetGround(false);
        }

        // 상태 초기화
        m_pOwner->ChangeState(PLAYER_STATE::IDLE);

        // 흡입 시스템 초기화
        if (m_pOwner->GetInhaleSystem())
        {
            m_pOwner->StopInhale();
            m_pOwner->ReleaseMouthful();
        }
    }
}

void CPlayerHealthSystem::StartInvincible(float _fTime)
{
    m_bIsInvincible = true;
    m_fInvincibleTime = _fTime;
    m_fInvincibleTimer = 0.0f;
}

// === 업데이트 함수들 ===

void CPlayerHealthSystem::UpdateInvincible()
{
    if (!m_bIsInvincible)
        return;

    m_fInvincibleTimer += CTimeMgr::GetInst()->GetfDT();

    if (m_fInvincibleTimer >= m_fInvincibleTime)
    {
        StopInvincible();
    }
}

void CPlayerHealthSystem::UpdateGameOver()
{
    m_fGameOverTimer += CTimeMgr::GetInst()->GetfDT();

    // 설정된 대기 시간 후 재시작
    if (m_fGameOverTimer >= m_fGameOverDelay)
    {
        RestartStage();
    }
}

void CPlayerHealthSystem::UpdateKnockback()
{
    if (!m_bKnockbackActive || !m_pOwner)
        return;

    m_fKnockbackTimer += CTimeMgr::GetInst()->GetfDT();

    // 넉백 지속 시간이 끝나면 정지
    if (m_fKnockbackTimer >= m_fKnockbackDuration)
    {
        m_bKnockbackActive = false;
        m_fKnockbackTimer = 0.0f;

        // 넉백 힘 제거
        CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
        if (pRigidBody)
        {
            Vec2 vCurrentVel = pRigidBody->GetVelocity();
            // Y축 속도는 유지 (중력 때문)
            pRigidBody->SetVelocity(Vec2(0.f, vCurrentVel.y));
        }
    }
}

void CPlayerHealthSystem::CheckGameOverConditions()
{
    if (!m_pOwner || m_bIsGameOver)
        return;

    // Y축 낙사 체크
    Vec2 vPos = m_pOwner->GetPos();
    if (vPos.y > m_fGameOverY)
    {
        ForceGameOver();
    }
}

// === 렌더링 함수들 ===

void CPlayerHealthSystem::RenderHealthUI(HDC _dc)
{
    // 화면 고정 위치에 체력 표시
    Vec2 vUIPos(50.f, 50.f);
    float fHeartSize = 28.f;
    float fHeartSpacing = 35.f;

    // 배경 사각형
    HBRUSH hBgBrush = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH hOldBgBrush = (HBRUSH)SelectObject(_dc, hBgBrush);

    RECT bgRect = {
        (LONG)(vUIPos.x - 10),
        (LONG)(vUIPos.y - 10),
        (LONG)(vUIPos.x + (m_iMaxHP * fHeartSpacing) + 10),
        (LONG)(vUIPos.y + fHeartSize + 40)
    };

    FillRect(_dc, &bgRect, hBgBrush);
    SelectObject(_dc, hOldBgBrush);
    DeleteObject(hBgBrush);

    // 하트 렌더링
    for (int i = 0; i < m_iMaxHP; ++i)
    {
        Vec2 vHeartPos = vUIPos + Vec2(i * fHeartSpacing, 0.f);

        COLORREF heartColor = (i < m_iCurrentHP) ?
            RGB(255, 100, 100) : RGB(100, 100, 100);

        HBRUSH hHeartBrush = CreateSolidBrush(heartColor);
        HBRUSH hOldHeartBrush = (HBRUSH)SelectObject(_dc, hHeartBrush);
        HPEN hHeartPen = CreatePen(PS_SOLID, 2, RGB(150, 50, 50));
        HPEN hOldHeartPen = (HPEN)SelectObject(_dc, hHeartPen);

        // 하트 모양 (간단한 원형)
        Ellipse(_dc,
            (int)(vHeartPos.x - fHeartSize / 2),
            (int)(vHeartPos.y - fHeartSize / 2),
            (int)(vHeartPos.x + fHeartSize / 2),
            (int)(vHeartPos.y + fHeartSize / 2));

        SelectObject(_dc, hOldHeartBrush);
        SelectObject(_dc, hOldHeartPen);
        DeleteObject(hHeartBrush);
        DeleteObject(hHeartPen);
    }

    // 체력 텍스트
    HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    SetTextColor(_dc, RGB(255, 255, 255));
    SetBkMode(_dc, TRANSPARENT);

    wchar_t szHP[32];
    swprintf_s(szHP, L"HP: %d/%d", m_iCurrentHP, m_iMaxHP);
    TextOut(_dc, (int)vUIPos.x, (int)(vUIPos.y + fHeartSize + 10), szHP, (int)wcslen(szHP));

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
}

void CPlayerHealthSystem::RenderGameOverUI(HDC _dc)
{
    RECT clientRect;
    GetClientRect(CCore::GetInst()->GetMainHwnd(), &clientRect);
    int screenWidth = clientRect.right - clientRect.left;
    int screenHeight = clientRect.bottom - clientRect.top;

    // 반투명 배경
    HBRUSH hBgBrush = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH hOldBgBrush = (HBRUSH)SelectObject(_dc, hBgBrush);
    Rectangle(_dc, 0, 0, screenWidth, screenHeight);
    SelectObject(_dc, hOldBgBrush);
    DeleteObject(hBgBrush);

    // "GAME OVER" 텍스트
    HFONT hTitleFont = CreateFont(64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldTitleFont = (HFONT)SelectObject(_dc, hTitleFont);

    SetTextColor(_dc, RGB(255, 50, 50));
    SetBkMode(_dc, TRANSPARENT);

    const wchar_t* szGameOver = L"GAME OVER";
    SIZE textSize;
    GetTextExtentPoint32(_dc, szGameOver, (int)wcslen(szGameOver), &textSize);

    int textX = (screenWidth - textSize.cx) / 2;
    int textY = (screenHeight - textSize.cy) / 2 - 40;
    TextOut(_dc, textX, textY, szGameOver, (int)wcslen(szGameOver));

    SelectObject(_dc, hOldTitleFont);
    DeleteObject(hTitleFont);

    // 재시작 카운트다운
    HFONT hSubFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldSubFont = (HFONT)SelectObject(_dc, hSubFont);

    SetTextColor(_dc, RGB(200, 200, 200));

    float fRemainingTime = m_fGameOverDelay - m_fGameOverTimer;
    wchar_t szCountdown[64];
    swprintf_s(szCountdown, L"Restarting in %.1f seconds...", fRemainingTime);

    GetTextExtentPoint32(_dc, szCountdown, (int)wcslen(szCountdown), &textSize);
    textX = (screenWidth - textSize.cx) / 2;
    textY = (screenHeight - textSize.cy) / 2 + 40;
    TextOut(_dc, textX, textY, szCountdown, (int)wcslen(szCountdown));

    SelectObject(_dc, hOldSubFont);
    DeleteObject(hSubFont);
}

// === 효과 함수들 ===

void CPlayerHealthSystem::ApplyKnockback(Vec2 _vDirection, float _fPower)
{
    if (!m_pOwner)
        return;

    CRigidBody* pRigidBody = m_pOwner->GetRigidBody();
    if (!pRigidBody)
        return;

    // 넉백 방향 정규화
    _vDirection.Normalize();

    // 넉백 힘 계산
    m_vKnockbackForce = _vDirection * _fPower;

    // 즉시 속도 적용
    pRigidBody->AddVelocity(m_vKnockbackForce);

    // 넉백 상태 시작
    m_bKnockbackActive = true;
    m_fKnockbackTimer = 0.0f;
}

void CPlayerHealthSystem::PlayDamageEffects()
{
    // TODO: 피격 효과음 재생
    // TODO: 화면 흔들림 효과
    // TODO: 파티클 효과 등
}

void CPlayerHealthSystem::PlayHealEffects()
{
    // TODO: 회복 효과음 재생
    // TODO: 회복 파티클 효과
}

void CPlayerHealthSystem::PlayGameOverEffects()
{
    // TODO: 게임오버 효과음 재생
    // TODO: 게임오버 애니메이션
}