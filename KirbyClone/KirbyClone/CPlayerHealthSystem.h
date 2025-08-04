#pragma once

class CPlayer;

class CPlayerHealthSystem
{
private:
    CPlayer* m_pOwner;              // 플레이어 참조

    // === 체력 관련 ===
    int m_iMaxHP;                   // 최대 체력 (6)
    int m_iCurrentHP;               // 현재 체력

    // === 무적 상태 관련 ===
    bool m_bIsInvincible;           // 무적 상태
    float m_fInvincibleTime;        // 무적 지속 시간
    float m_fInvincibleTimer;       // 무적 타이머
    float m_fBlinkInterval;         // 깜빡임 간격

    // === 게임오버 관련 ===
    bool m_bIsGameOver;             // 게임오버 상태
    float m_fGameOverY;             // 낙사 게임오버 Y 좌표
    float m_fGameOverTimer;         // 게임오버 후 대기 시간
    float m_fGameOverDelay;         // 재시작 대기 시간 (2초)

    // === 넉백 관련 ===
    bool m_bKnockbackActive;        // 넉백 진행중
    float m_fKnockbackTimer;        // 넉백 지속 시간
    float m_fKnockbackDuration;     // 넉백 총 지속 시간
    Vec2 m_vKnockbackForce;         // 넉백 힘

public:
    CPlayerHealthSystem(CPlayer* _pOwner);
    ~CPlayerHealthSystem();

public:
    void Init();
    void Update();
    void Render(HDC _dc);

    // === 체력 관리 ===
    void TakeDamage(int _iDamage = 1, Vec2 _vKnockbackDir = Vec2(0.f, 0.f));
    void Heal(int _iHeal = 1);
    void SetHP(int _iHP);
    void SetMaxHP(int _iMaxHP);

    // === 게터 함수들 ===
    int GetCurrentHP() const { return m_iCurrentHP; }
    int GetMaxHP() const { return m_iMaxHP; }
    float GetHPRatio() const { return (float)m_iCurrentHP / (float)m_iMaxHP; }

    // === 상태 체크 ===
    bool IsInvincible() const { return m_bIsInvincible; }
    bool IsGameOver() const { return m_bIsGameOver; }
    bool IsKnockbackActive() const { return m_bKnockbackActive; }
    bool ShouldRenderBlink() const;     // 깜빡임 렌더링 여부

    // === 게임오버 관련 ===
    void SetGameOverY(float _fY) { m_fGameOverY = _fY; }
    void ForceGameOver();               // 강제 게임오버
    void RestartStage();                // 스테이지 재시작

    // === 무적 관련 ===
    void StartInvincible(float _fTime = 2.0f);
    void StopInvincible() { m_bIsInvincible = false; m_fInvincibleTimer = 0.f; }

private:
    // === 업데이트 함수들 ===
    void UpdateInvincible();           // 무적 시간 업데이트
    void UpdateGameOver();             // 게임오버 처리
    void UpdateKnockback();            // 넉백 처리
    void CheckGameOverConditions();    // 게임오버 조건 체크

    // === 렌더링 함수들 ===
    void RenderHealthUI(HDC _dc);      // 체력 UI 렌더링
    void RenderGameOverUI(HDC _dc);    // 게임오버 UI 렌더링

    // === 효과 함수들 ===
    void ApplyKnockback(Vec2 _vDirection, float _fPower = 200.f);
    void PlayDamageEffects();          // 피격 효과 (사운드, 화면 흔들림 등)
    void PlayHealEffects();            // 회복 효과
    void PlayGameOverEffects();        // 게임오버 효과
};