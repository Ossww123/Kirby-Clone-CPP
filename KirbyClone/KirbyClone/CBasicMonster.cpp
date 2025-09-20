#include "gamePCH.h"
#include "CBasicMonster.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CAnimator.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"
#include "CPlayerInhaleSystem.h"
#include "CSoundMgr.h"
#include "CDeathEffect.h"

CBasicMonster::CBasicMonster()
    : CMonster()
    , m_bBeingInhaled(false)
    , m_fInhaleForce(0.f)
    , m_vPlayerPos(Vec2(0.f, 0.f))
    , m_vDamageSourcePos(Vec2(0.f, 0.f))
    , m_bPlayerDetected(false)
    , m_fDetectionRange(100.f)
    , m_iHealth(1)                  // 기본 몬스터 체력 = 1
    , m_iMaxHealth(1)               // 최대 체력 = 1
    , m_bIsDying(false)             // 죽는 중 상태 초기화
    , m_fDeathEffectTimer(0.f)      // 죽음 효과 타이머 초기화
    , m_vKnockbackDir(Vec2(0.f, 0.f)) // 넉백 방향 초기화
    , m_fKnockbackSpeed(0.f)        // 넉백 속도 초기화
{
    // 일반 몬스터는 빨아들임 가능하므로 특별한 설정 불필요
}

CBasicMonster::~CBasicMonster()
{
    // 상위 클래스에서 정리 처리
}

void CBasicMonster::OnInhaled()
{
    // 플레이어의 InhaleSystem에게 삼키기 요청
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayers.empty())
        {
            CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
            if (pPlayer && pPlayer->GetInhaleSystem())
            {
                pPlayer->GetInhaleSystem()->SwallowTarget(this);
            }
        }
    }

    // TODO: 이펙트 생성
    // TODO: 사운드 재생
}

void CBasicMonster::OnInhaleStart()
{
    // 빨아들임 시작 시 처리
    m_bBeingInhaled = true;

    // 빨아들임 중에는 일반 AI 정지
    if (GetCurrentState() != MONSTER_STATE::BEING_INHALED)
    {
        ChangeState(MONSTER_STATE::BEING_INHALED);  // 빨아들려지는 상태로 전환
    }
}

void CBasicMonster::CheckPlayerDistance()
{
    // 플레이어와의 거리 체크
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (nullptr == pCurScene)
        return;

    const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
    
    if (!vecPlayers.empty())
    {
        CPlayer* pPlayer = (CPlayer*)vecPlayers[0];  // 플레이어는 1명
        m_vPlayerPos = pPlayer->GetPos();

        Vec2 vDist = m_vPlayerPos - GetPos();
        float fDistance = vDist.Length();

        // AI용 플레이어 감지
        m_bPlayerDetected = (fDistance <= m_fDetectionRange);
    }
}

void CBasicMonster::HandleInhaleEffect()
{
    if (!m_bBeingInhaled)
        return;

    // 플레이어 쪽으로 끌려가는 처리
    ProcessInhaleMovement();

    // 플레이어와 너무 가까워지면 흡수
    CheckInhaleDistance();
}

void CBasicMonster::UpdateInhaleState()
{
    if (m_bBeingInhaled)
    {
        HandleInhaleEffect();
    }
    else
    {
        CheckPlayerDistance();
    }
}

void CBasicMonster::Update()
{
    // 죽음 효과 처리 먼저 (최우선)
    UpdateDeathEffect();

    // 죽는 중이면 AI만 중단하고 물리 업데이트는 계속
    if (m_bIsDying)
    {
        // 물리 업데이트만 수행 (RigidBody, Animator)
        if (nullptr != GetRigidBody())
            GetRigidBody()->Update();
        
        if (nullptr != GetAnimator())
            GetAnimator()->Update();
        
        // 상태 타이머도 업데이트 (CMonster에서 사용)
        m_fStateTimer += CTimeMgr::GetInst()->GetfDT();
        return;
    }

    // 부모 클래스의 일반 업데이트 호출
    CMonster::Update();
}

void CBasicMonster::UpdateIdle()
{
    if (m_bBeingInhaled)
        return;  // 빨아들임 중이면 일반 AI 중단

    // 플레이어 감지 (AI용)
    CheckPlayerDistance();

    // 부모 클래스의 기본 Idle 처리
    CMonster::UpdateIdle();
}

void CBasicMonster::UpdateWalk()
{
    // 빨아들임 상태 체크 먼저
    UpdateInhaleState();

    if (m_bBeingInhaled)
        return;  // 빨아들임 중이면 일반 AI 중단

    // 부모 클래스의 기본 Walk 처리
    CMonster::UpdateWalk();
}

void CBasicMonster::UpdateFly()
{
    // 빨아들임 상태 체크 먼저
    UpdateInhaleState();

    if (m_bBeingInhaled)
        return;  // 빨아들임 중이면 일반 AI 중단

    // 부모 클래스의 기본 Fly 처리
    CMonster::UpdateFly();
}

void CBasicMonster::UpdateTurn()
{
    // 빨아들임 상태 체크 먼저
    UpdateInhaleState();

    if (m_bBeingInhaled)
        return;  // 빨아들임 중이면 일반 AI 중단

    // 부모 클래스의 기본 Turn 처리
    CMonster::UpdateTurn();
}

void CBasicMonster::ProcessInhaleMovement()
{
    // CPlayerInhaleSystem에서 이동을 담당하므로 여기서는 처리하지 않음
    // 단지 플레이어 위치만 업데이트
}

void CBasicMonster::CheckInhaleDistance()
{
    Vec2 vDist = m_vPlayerPos - GetPos();
    float fDistance = vDist.Length();

    // 플레이어와 매우 가까워지면 흡수 처리
    if (fDistance <= 30.f)  // 흡수 거리
    {
        OnInhaled();
    }
}

void CBasicMonster::TakeDamage()
{
    // 이미 죽는 중이거나 빨아들임 중이면 데미지 무시
    if (m_bIsDying || m_bBeingInhaled)
        return;

    // 체력 감소
    m_iHealth--;

    // 몬스터 데미지 사운드 재생
    CSoundMgr::GetInst ( )->PlaySFX ( L"monster_damage" );

    // 체력이 0 이하이면 죽음 효과 시작
    if (m_iHealth <= 0)
    {
        // 넉백 방향 계산 (데미지 소스 반대 방향)
        Vec2 vMyPos = GetPos();
        Vec2 vKnockbackDir = vMyPos - m_vDamageSourcePos;
        float fDistance = vKnockbackDir.Length();
        
        if (fDistance < 0.1f)  // 거의 같은 위치면 랜덤 방향
        {
            vKnockbackDir = Vec2(1.f, 0.f);  // 기본적으로 오른쪽
        }
        else
        {
            vKnockbackDir.Normalize();  // 정규화
        }

        StartDeathEffect(vKnockbackDir);
    }
    else
    {
        // 체력이 남아있으면 일반 데미지 처리
        CMonster::TakeDamage();  // 부모 클래스의 TakeDamage 호출
    }
}

void CBasicMonster::StartDeathEffect(Vec2 _vKnockbackDir)
{
    m_bIsDying = true;
    m_fDeathEffectTimer = 0.f;
    m_vKnockbackDir = _vKnockbackDir;
    m_fKnockbackSpeed = 500.f;  // 넉백 초기 속도 (더 명확한 넉백을 위해 증가)

    // 넉백 중에는 중력 비활성화
    if (GetRigidBody())
    {
        GetRigidBody()->SetUseGravity(false);
    }

    // 죽음 상태로 변경
    ChangeState(MONSTER_STATE::DAMAGE);
}

void CBasicMonster::UpdateDeathEffect()
{
    if (!m_bIsDying)
        return;

    Vec2 vPosBeforeUpdate = GetPos();
    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fDeathEffectTimer += fDT;

    // 넉백 이동 처리 (감속하면서)
    if (m_fKnockbackSpeed > 0.f)
    {
        // 감속 적용
        m_fKnockbackSpeed -= 800.f * fDT;  // 초당 800픽셀/초씩 감속
        if (m_fKnockbackSpeed < 0.f)
            m_fKnockbackSpeed = 0.f;

        // 넉백 이동 (물리 체계 사용 여부 체크)
        if (GetRigidBody())
        {
            // RigidBody가 있으면 속도로 설정
            Vec2 vKnockbackVel = m_vKnockbackDir * m_fKnockbackSpeed;
            GetRigidBody()->SetVelocity(vKnockbackVel);
            
            // 설정 후 실제 속도 확인
            Vec2 vCurrentVel = GetRigidBody()->GetVelocity();
        }
        else
        {
            // RigidBody가 없으면 직접 위치 이동
            Vec2 vCurrentPos = GetPos();
            Vec2 vMovement = m_vKnockbackDir * m_fKnockbackSpeed * fDT;
            SetPos(vCurrentPos + vMovement);
        }
    }

    // 1.0초 후 실제로 죽음 처리 (넉백을 더 명확하게 보기 위해 증가)
    if (m_fDeathEffectTimer >= 1.0f)
    {
        // 죽음 이펙트 생성
        CDeathEffect* pDeathEffect = new CDeathEffect();
        pDeathEffect->SetPos(GetPos());
        pDeathEffect->Init();
        
        // 현재 씬에 이펙트 추가
        CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurScene)
        {
            pCurScene->AddObject(pDeathEffect, GROUP_TYPE::EFFECT);
        }
        
        // 적 사망 효과음 (실제 사라지기 직전)
        CSoundMgr::GetInst()->PlaySFX(L"enemy_death");
        SetDead();
    }
}

void CBasicMonster::UpdateDamage()
{
    // 죽는 중이면 넉백 효과만 처리하고 부모 클래스 호출하지 않음
    if (m_bIsDying)
    {
        UpdateDeathEffect();
        return;
    }

    // 일반 데미지는 부모 클래스에서 처리
    CMonster::UpdateDamage();
}

void CBasicMonster::UpdateBeingInhaled()
{
    // 빨아들려지는 중에는 일반 AI 완전 정지
    // 물리 이동은 CPlayerInhaleSystem에서 처리됨
    
    // 타일과의 충돌 비활성화 및 중력 비활성화
    if (GetRigidBody())
    {
        GetRigidBody()->SetUseGravity(false);  // 빨아들려지는 중에는 중력 비활성화
    }
}