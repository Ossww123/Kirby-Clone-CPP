#include "pch.h"
#include "CBoss.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CEventMgr.h"
#include "CSceneMgr.h"
#include "CApple.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CPlayerStateMachine.h"
#include "CCamera.h"
#include "CSoundMgr.h"

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
    , m_eDefeatPhase(BOSS_DEFEAT_PHASE::NONE)
    , m_fDefeat1Timer(0.f)
    , m_fVictoryWaitTimer(0.f)
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

void CBoss::StartBossEvent()
{
    // 보스 이벤트 시작으로 설정
    m_bBossEventStarted = true;
    
    // BATTLE 페이즈로 변경
    SetBossPhase(BOSS_PHASE::PHASE_1 );
    
    // 공격 패턴 초기화
    m_fPatternTimer = 0.f;
    m_iPatternCount = 0;
    
    // IDLE 상태로 변경하여 공격 시작 준비
    ChangeState(MONSTER_STATE::IDLE);
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

    // 보스 피격 카메라 흔들림 효과 (0.4초간 20픽셀 강도)
    CCamera::GetInst()->CameraShake(0.4f, 20.f);

    // 몬스터 데미지 사운드 재생
    CSoundMgr::GetInst ( )->PlaySFX ( L"monster_damage" );

    // 데미지 후 짧은 무적 시간
    m_bInvincible = true;
    m_fInvincibleTime = 0.2f;

    // 페이즈 전환 체크
    CheckPhaseTransition();

    // 패배 체크
    if (IsDefeated())
    {
        SetBossPhase(BOSS_PHASE::DEFEATED);
        
        // 현재 씬의 BGM 정지
        CSoundMgr::GetInst()->StopBGM();
        
        // 격파 시퀀스 시작
        m_eDefeatPhase = BOSS_DEFEAT_PHASE::DEFEAT1_ANIM;
        m_fDefeat1Timer = 0.f;
        
        // 씬 전체 일시정지 (2초간 모든 오브젝트 업데이트 중단)
        CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurrentScene)
        {
            pCurrentScene->SetPaused(true);
        }
        
        // STAGE_CLEAR 이벤트 발생
        tEvent event = {};
        event.eType = EVENT_TYPE::STAGE_CLEAR;
        event.lParam = (DWORD_PTR)this;
        event.wParam = 0;
        CEventMgr::GetInst()->AddEvent(event);
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
        }
        break;

    case BOSS_PHASE::PHASE_1:
    case BOSS_PHASE::PHASE_2:
    case BOSS_PHASE::PHASE_3:
        UpdateAttackPattern();
        break;

    case BOSS_PHASE::DEFEATED:
        UpdateDefeatSequence();
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

void CBoss::Update()
{
    // DEFEATED 페이즈에서는 격파 시퀀스만 업데이트
    if (m_eBossPhase == BOSS_PHASE::DEFEATED)
    {
        UpdateDefeatSequence();
        return;
    }
    
    // 그 외에는 일반 몬스터 업데이트
    CMonster::Update();
}

void CBoss::UpdateIdle()
{
    // 보스는 Idle 상태에서도 페이즈 업데이트
    UpdateBossPhase();
}

void CBoss::UpdateAttackReady()
{
    // 패배 상태에서는 공격 준비를 중단
    if (m_eBossPhase == BOSS_PHASE::DEFEATED)
    {
        ChangeState(MONSTER_STATE::IDLE);
        return;
    }

    // 보스는 공격 준비 시간이 짧음
    if (m_fStateTimer >= 0.5f)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }
}

void CBoss::UpdateAttack()
{
    // 패배 상태에서는 공격을 즉시 중단
    if (m_eBossPhase == BOSS_PHASE::DEFEATED)
    {
        ChangeState(MONSTER_STATE::IDLE);
        return;
    }

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

void CBoss::UpdateDefeatSequence()
{
    switch (m_eDefeatPhase)
    {
    case BOSS_DEFEAT_PHASE::DEFEAT1_ANIM:
        // DEFEAT1 상태로 변경 (애니메이션 재생)
        ChangeState(MONSTER_STATE::DEFEAT1);
        
        // DEFEAT1 애니메이션 실행 중 (2초 동안)
        m_fDefeat1Timer += CTimeMgr::GetInst()->GetfDT();
        
        // 2초가 지나면 사과와 투사체들 정리하고 커비 움직임 제한 해제
        if (m_fDefeat1Timer >= 2.0f)
        {
            CleanupAppleAndProjectiles();
            ReleaseBossDefeatWaiting();
            
            // 씬 일시정지 해제
            CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
            if (pCurrentScene)
            {
                pCurrentScene->SetPaused(false);
            }
            
            m_eDefeatPhase = BOSS_DEFEAT_PHASE::DEFEAT3_ANIM;
        }
        break;
        
    case BOSS_DEFEAT_PHASE::DEFEAT3_ANIM:
        // DEFEAT3 상태로 변경 (애니메이션 재생)
        ChangeState(MONSTER_STATE::DEFEAT3);
        
        // 커비를 화면 가운데로 이동시키고 승리 시퀀스 입력 차단
        MovePlayerToCenterAndStartVictoryWait();
        
        m_eDefeatPhase = BOSS_DEFEAT_PHASE::VICTORY_WAIT;
        break;
        
    case BOSS_DEFEAT_PHASE::VICTORY_WAIT:
        // 승리 대기 중 (2초간 커비가 안착할 시간 제공)
        m_fVictoryWaitTimer += CTimeMgr::GetInst()->GetfDT();
        
        // 2초 후 승리 춤 시작
        if (m_fVictoryWaitTimer >= 2.0f)
        {
            StartVictoryDance();
            m_eDefeatPhase = BOSS_DEFEAT_PHASE::VICTORY_DANCE;
        }
        break;
        
    case BOSS_DEFEAT_PHASE::VICTORY_DANCE:
        // 커비 승리 춤 진행 중 (CPlayerStateMachine에서 처리)
        break;
    }
}

void CBoss::CleanupAppleAndProjectiles()
{
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurrentScene)
        return;
    
    // 사과 오브젝트들 정리 (사과는 MONSTER 그룹에 있음)
    const vector<CObject*>& vecMonsters = pCurrentScene->GetGroupObject(GROUP_TYPE::MONSTER);
    for (CObject* pObj : vecMonsters)
    {
        // 사과 오브젝트인지 확인 (dynamic_cast 사용)
        if (pObj && dynamic_cast<CApple*>(pObj))
        {
            pObj->SetDead();
        }
    }
    
    // 투사체들 정리 (몬스터 투사체 그룹)
    const vector<CObject*>& vecMonsterProjectiles = pCurrentScene->GetGroupObject(GROUP_TYPE::PROJ_MONSTER);
    for (CObject* pObj : vecMonsterProjectiles)
    {
        if (pObj)
        {
            pObj->SetDead();
        }
    }
    
    // 플레이어 투사체들도 정리
    const vector<CObject*>& vecPlayerProjectiles = pCurrentScene->GetGroupObject(GROUP_TYPE::PROJ_PLAYER);
    for (CObject* pObj : vecPlayerProjectiles)
    {
        if (pObj)
        {
            pObj->SetDead();
        }
    }
}

void CBoss::ReleaseBossDefeatWaiting()
{
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurrentScene)
        return;
    
    // 플레이어의 보스 격파 대기 상태 해제
    const vector<CObject*>& vecPlayers = pCurrentScene->GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayers.empty() && vecPlayers[0])
    {
        CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
        pPlayer->SetBossDefeatWaiting(false);  // 커비 움직임 제한 해제
    }
}

void CBoss::MovePlayerToCenterAndStartVictoryWait()
{
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurrentScene)
        return;
    
    // 플레이어 찾기
    const vector<CObject*>& vecPlayers = pCurrentScene->GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayers.empty() && vecPlayers[0])
    {
        CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
        
        // 카메라 기준 화면 가운데 위치 계산
        Vec2 cameraPos = CCamera::GetInst()->GetLookAt();
        Vec2 centerPos = Vec2(cameraPos.x, cameraPos.y + 192.f);  // 카메라 가운데
        
        // 커비를 해당 위치로 강제 이동
        pPlayer->SetPos(centerPos);
        
        // 승리 시퀀스 입력 차단 시작
        pPlayer->SetVictorySequenceWaiting(true);
        
        // 승리 대기 타이머 초기화
        m_fVictoryWaitTimer = 0.f;
    }
}

void CBoss::StartVictoryDance()
{
    OutputDebugString(L"[DEBUG] StartVictoryDance() called\n");
    
    CScene* pCurrentScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurrentScene)
    {
        OutputDebugString(L"[DEBUG] ERROR: No current scene!\n");
        return;
    }
    
    // 플레이어 찾기
    const vector<CObject*>& vecPlayers = pCurrentScene->GetGroupObject(GROUP_TYPE::PLAYER);
    if (!vecPlayers.empty() && vecPlayers[0])
    {
        CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
        
        OutputDebugString(L"[DEBUG] Player found, releasing victory sequence waiting\n");
        
        // 승리 시퀀스 입력 차단 해제
        pPlayer->SetVictorySequenceWaiting(false);
        
        OutputDebugString(L"[DEBUG] Forcing state change to VICTORY_DANCE\n");
        
        // 승리 춤 상태로 변경 (StateMachine을 통해 강제 전환)
        pPlayer->GetStateMachine()->ForceStateChange(PLAYER_STATE::VICTORY_DANCE);
        
        OutputDebugString(L"[DEBUG] VICTORY_DANCE state change completed\n");
    }
    else
    {
        OutputDebugString(L"[DEBUG] ERROR: No player found!\n");
    }
}