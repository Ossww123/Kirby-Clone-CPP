#pragma once
#include "CMonster.h"

enum class BOSS_PHASE
{
    INTRO,          // 등장 연출
    PHASE_1,        // 1페이즈
    PHASE_2,        // 2페이즈 (체력 50% 이하)
    PHASE_3,        // 3페이즈 (체력 25% 이하)
    DEFEATED,       // 패배 연출
    END
};

enum class BOSS_ATTACK_PATTERN
{
    PATTERN_1,
    PATTERN_2,
    PATTERN_3,
    PATTERN_4,
    PATTERN_5,
    END
};

enum class BOSS_DEFEAT_PHASE
{
    NONE,           // 아직 격파되지 않음
    DEFEAT1_ANIM,   // DEFEAT1 애니메이션 재생 중 (2초)
    DEFEAT3_ANIM,   // DEFEAT3 애니메이션 계속 재생
    VICTORY_WAIT,   // 커비 가운데 이동 후 대기 (2초)
    VICTORY_DANCE   // 승리 춤 단계
};

class CBoss : public CMonster
{
public:
    CBoss();
    virtual ~CBoss();

public:
    // === 보스 특성 (final로 하위 클래스에서 변경 불가) ===
    bool CanBeInhaled() const override final { return false; }
    bool IsBeingInhaled() const override final { return false; }
    bool HasAttack() const override { return true; }
    virtual bool IsBoss() const override final { return true; }  // 보스 여부 확인

public:
    // === 순수 가상 함수 (하위 클래스에서 반드시 구현) ===
    virtual void EndBossEvent() = 0;                        // 보스전 종료
    virtual void ExecuteAttackPattern(BOSS_ATTACK_PATTERN _ePattern) = 0;  // 공격 패턴 실행

public:
    // === 보스 상태 관리 ===
    void SetBossPhase(BOSS_PHASE _ePhase);
    BOSS_PHASE GetBossPhase() const { return m_eBossPhase; }

    // === 보스 체력 관리 ===
    void SetBossHP(int _iHP) { m_iMaxHP = _iHP; m_iCurrentHP = _iHP; }
    int GetCurrentHP() const { return m_iCurrentHP; }
    int GetMaxHP() const { return m_iMaxHP; }
    float GetHPRatio() const { return (float)m_iCurrentHP / (float)m_iMaxHP; }

    // === 보스 데미지 처리 ===
    void TakeBossDamage(int _iDamage);
    bool IsDefeated() const { return m_iCurrentHP <= 0; }
    
    // === 보스 이벤트 제어 ===
    virtual void StartBossEvent();                          // 보스전 시작 (오버라이드 가능)
    bool IsBossEventStarted() const { return m_bBossEventStarted; }

protected:
    // === 보스 시스템 관리 ===
    void UpdateBossPhase();                                 // 보스 페이즈 관리
    void UpdateAttackPattern();                             // 공격 패턴 관리
    void SelectNextAttackPattern();                         // 다음 공격 패턴 선택

    // === 페이즈 전환 처리 ===
    void CheckPhaseTransition();                            // 페이즈 전환 체크
    void OnPhaseChanged(BOSS_PHASE _eNewPhase);            // 페이즈 변경 시 처리

    // === 상태 업데이트 오버라이드 ===
    void Update() override;                                 // 메인 업데이트 (DEFEATED 페이즈 처리)
    void UpdateIdle() override;
    void UpdateAttackReady() override;
    void UpdateAttack() override;

    // === 보스 전용 데미지 처리 ===
    void TakeDamage() override;                             // 일반 데미지는 무시

protected:
    // === 보스 유틸리티 함수들 ===
    void CreateBossIntroEffect();                           // 등장 이펙트
    void CreateBossDefeatedEffect();                        // 패배 이펙트
    void ShowPhaseChangeEffect();                           // 페이즈 변경 이펙트
    void PlayBossMusic();                                   // 보스 BGM 재생
    void StopBossMusic();                                   // 보스 BGM 정지
    
    // === 격파 시퀀스 관리 ===
    void UpdateDefeatSequence();                            // 격파 시퀀스 업데이트
    void CleanupAppleAndProjectiles();                      // 사과와 투사체 정리
    void ReleaseBossDefeatWaiting();                        // 커비 움직임 제한 해제
    void MovePlayerToCenterAndStartVictoryWait();           // 커비 이동 및 승리 대기 시작
    void StartVictoryDance();                               // 승리 춤 시작

private:
    // === 보스 기본 정보 ===
    BOSS_PHASE          m_eBossPhase;                       // 현재 보스 페이즈
    int                 m_iCurrentHP;                       // 현재 체력
    int                 m_iMaxHP;                           // 최대 체력

    // === 공격 패턴 관리 ===
    BOSS_ATTACK_PATTERN m_eCurrentPattern;                 // 현재 공격 패턴
    float               m_fPatternTimer;                    // 패턴 타이머
    float               m_fPatternDuration;                 // 패턴 지속 시간
    int                 m_iPatternCount;                    // 패턴 실행 횟수

    // === 보스 이벤트 관리 ===
    bool                m_bBossEventStarted;               // 보스전 시작 여부
    bool                m_bBossEventEnded;                 // 보스전 종료 여부
    float               m_fIntroTimer;                      // 인트로 타이머
    float               m_fDefeatedTimer;                   // 패배 연출 타이머

    // === 무적 시간 관리 ===
    float               m_fInvincibleTime;                  // 무적 시간
    bool                m_bInvincible;                      // 무적 상태
    
    // === 격파 시퀀스 관리 ===
    BOSS_DEFEAT_PHASE   m_eDefeatPhase;                     // 격파 단계
    float               m_fDefeat1Timer;                    // DEFEAT1 애니메이션 타이머
    float               m_fVictoryWaitTimer;                // 승리 대기 타이머
    bool                m_bBossClearSoundPlayed;            // 보스 클리어 사운드 재생 플래그
};
