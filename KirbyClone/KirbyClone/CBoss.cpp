#include "pch.h"
#include "CBoss.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"

CBoss::CBoss()
    : CMonster()
    , m_eBossPhase(BOSS_PHASE::INTRO)
    , m_iCurrentHP(1000)
    , m_iMaxHP(1000)
    , m_eCurrentPattern(BOSS_ATTACK_PATTERN::PATTERN_1)
    , m_fPatternTimer(0.f)
    , m_fPatternDuration(5.f)
    , m_iPatternCount(0)
    , m_bBossEventStarted(false)
    , m_bBossEventEnded(false)
    , m_fIntroTimer(0.f)
    , m_fDefeatedTimer(0.f)
    , m_fInvincibleTime(0.f)
    , m_bInvincible(false)
{
    // 보스는 특별한 초기 설정
    m_fSpeed = 0.f;  // 기본적으로 이동하지 않음

    // 보스는 중력 영향 받지 않음
    GetRigidBody()->SetUseGravity(false);

    // 보스는 매우 큰 질량
    GetRigidBody()->SetMass(10.f);
}

CBoss::~CBoss()
{
    // 상위 클래스에서 정리 처리
}

void CBoss::SetBossPhase(BOSS_PHASE _ePhase)
{
    if (m_eBossPhase != _ePhase)
    {
        BOSS_PHASE prevPhase = m_eBossPhase;
        m_eBossPhase = _ePhase;
        OnPhaseChanged(_ePhase);
    }
}

void CBoss::TakeBossDamage(int _iDamage)
{
    if (m_bInvincible || IsDefeated())
        return;

    m_iCurrentHP -= _iDamage;
    if (m_iCurrentHP < 0)
        m_iCurrentHP = 0;

    // 데미지 후 짧은 무적 시간
    m_bInvincible = true;
    m_fInvincibleTime = 0.5f;

    // 페이즈 전환 체크
    CheckPhaseTransition();

    // 패배 체크
    if (IsDefeated())
    {
        SetBossPhase(BOSS_PHASE::DEFEATED);
    }
}

void CBoss::UpdateBossPhase()
{
    switch (m_eBossPhase)
    {
    case BOSS_PHASE::INTRO:
        m_fIntroTimer += CTimeMgr::GetInst()->GetfDT();
        if (m_fIntroTimer >= 3.f)  // 3초 인트로
        {
            SetBossPhase(BOSS_PHASE::PHASE_1);
            StartBossEvent();
        }
        break;

    case BOSS_PHASE::PHASE_1:
    case BOSS_PHASE::PHASE_2:
    case BOSS_PHASE::PHASE_3:
        UpdateAttackPattern();
        break;

    case BOSS_PHASE::DEFEATED:
        m_fDefeatedTimer += CTimeMgr::GetInst()->GetfDT();
        if (m_fDefeatedTimer >= 5.f)  // 5초 패배 연출
        {
            EndBossEvent();
            SetDead();
        }
        break;
    }

    // 무적 시간 업데이트
    if (m_bInvincible)
    {
        m_fInvincibleTime -= CTimeMgr::GetInst()->GetfDT();
        if (m_fInvincibleTime <= 0.f)
        {
            m_bInvincible = false;
        }
    }
}

void CBoss::UpdateAttackPattern()
{
    m_fPatternTimer += CTimeMgr::GetInst()->GetfDT();

    if (m_fPatternTimer >= m_fPatternDuration)
    {
        SelectNextAttackPattern();
        m_fPatternTimer = 0.f;
    }
}

void CBoss::SelectNextAttackPattern()
{
    // 현재 페이즈에 따른 패턴 선택
    switch (m_eBossPhase)
    {
    case BOSS_PHASE::PHASE_1:
        // 1페이즈: 기본 패턴들
        m_eCurrentPattern = (BOSS_ATTACK_PATTERN)((int)m_eCurrentPattern + 1);
        if (m_eCurrentPattern >= BOSS_ATTACK_PATTERN::PATTERN_3)
            m_eCurrentPattern = BOSS_ATTACK_PATTERN::PATTERN_1;
        break;

    case BOSS_PHASE::PHASE_2:
        // 2페이즈: 더 다양한 패턴
        m_eCurrentPattern = (BOSS_ATTACK_PATTERN)(rand() % 4);  // 패턴 1~4
        break;

    case BOSS_PHASE::PHASE_3:
        // 3페이즈: 모든 패턴 + 랜덤
        m_eCurrentPattern = (BOSS_ATTACK_PATTERN)(rand() % 5);  // 패턴 1~5
        break;
    }

    // 패턴 실행
    ExecuteAttackPattern(m_eCurrentPattern);
    m_iPatternCount++;
}

void CBoss::CheckPhaseTransition()
{
    float hpRatio = GetHPRatio();

    // 체력에 따른 페이즈 전환
    if (hpRatio <= 0.25f && m_eBossPhase != BOSS_PHASE::PHASE_3)
    {
        SetBossPhase(BOSS_PHASE::PHASE_3);
    }
    else if (hpRatio <= 0.5f && m_eBossPhase == BOSS_PHASE::PHASE_1)
    {
        SetBossPhase(BOSS_PHASE::PHASE_2);
    }
}

void CBoss::OnPhaseChanged(BOSS_PHASE _eNewPhase)
{
    // 페이즈 변경 시 처리
    ShowPhaseChangeEffect();

    // 패턴 타이머 리셋
    m_fPatternTimer = 0.f;
    m_iPatternCount = 0;

    // 페이즈별 특별 처리
    switch (_eNewPhase)
    {
    case BOSS_PHASE::PHASE_1:
        PlayBossMusic();
        m_fPatternDuration = 5.f;  // 5초 간격
        break;

    case BOSS_PHASE::PHASE_2:
        m_fPatternDuration = 4.f;  // 4초 간격 (더 빠름)
        break;

    case BOSS_PHASE::PHASE_3:
        m_fPatternDuration = 3.f;  // 3초 간격 (매우 빠름)
        break;

    case BOSS_PHASE::DEFEATED:
        StopBossMusic();
        CreateBossDefeatedEffect();
        break;
    }
}

void CBoss::UpdateIdle()
{
    // 보스는 Idle 상태에서도 페이즈 업데이트
    UpdateBossPhase();
}

void CBoss::UpdateAttackReady()
{
    // 보스는 공격 준비 시간이 짧음
    if (m_fStateTimer >= 0.5f)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }
}

void CBoss::UpdateAttack()
{
    // 보스는 공격 중에도 페이즈 업데이트
    UpdateBossPhase();

    if (m_fStateTimer >= 2.f)  // 2초 공격 지속
    {
        ChangeState(MONSTER_STATE::IDLE);
    }
}

void CBoss::TakeDamage()
{
    // 보스는 일반 데미지 무시
    // TakeBossDamage()만 유효
}

void CBoss::CreateBossIntroEffect()
{
    // 보스 등장 이펙트
    // TODO: 등장 이펙트 구현
}

void CBoss::CreateBossDefeatedEffect()
{
    // 보스 패배 이펙트
    // TODO: 패배 이펙트 구현
}

void CBoss::ShowPhaseChangeEffect()
{
    // 페이즈 변경 이펙트
    // TODO: 페이즈 변경 이펙트 구현
}

void CBoss::PlayBossMusic()
{
    // 보스 BGM 재생
    // TODO: 사운드 매니저를 통한 보스 BGM 재생
}

void CBoss::StopBossMusic()
{
    // 보스 BGM 정지
    // TODO: 사운드 매니저를 통한 보스 BGM 정지
}