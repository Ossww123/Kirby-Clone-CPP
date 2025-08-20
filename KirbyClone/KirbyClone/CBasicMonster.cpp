#include "pch.h"
#include "CBasicMonster.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CPlayer.h"

CBasicMonster::CBasicMonster()
    : CMonster()
    , m_bBeingInhaled(false)
    , m_fInhaleForce(0.f)
    , m_vPlayerPos(Vec2(0.f, 0.f))
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
    // 몬스터 제거
    SetDead();

    // TODO: 이펙트 생성
    // TODO: 사운드 재생
}

void CBasicMonster::OnInhaleStart()
{
    // 빨아들임 시작 시 처리
    m_bBeingInhaled = true;

    // 빨아들임 중에는 일반 AI 정지
    if (GetCurrentState() != MONSTER_STATE::DAMAGE)
    {
        ChangeState(MONSTER_STATE::DAMAGE);  // 임시로 damage 상태 사용
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

    // 죽는 중이면 다른 업데이트 중단
    if (m_bIsDying)
        return;

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
    // 플레이어 쪽으로 끌려가는 이동 처리
    Vec2 vCurrentPos = GetPos();
    Vec2 vDirection = m_vPlayerPos - vCurrentPos;

    if (vDirection.Length() > 0.1f)
    {
        vDirection.Normalize();

        // 빨아들임 힘 적용
        m_fInhaleForce += 100.f * CTimeMgr::GetInst()->GetfDT();  // 점점 강해짐
        float fMoveSpeed = m_fInhaleForce;

        // 리지드바디를 통한 이동
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocity(vDirection * fMoveSpeed);
        }
    }
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

    // 체력이 0 이하이면 죽음 효과 시작
    if (m_iHealth <= 0)
    {
        // 넉백 방향 계산 (플레이어 반대 방향)
        Vec2 vKnockbackDir = GetPos() - m_vPlayerPos;
        if (vKnockbackDir.Length() < 0.1f)  // 거의 같은 위치면 랜덤 방향
        {
            vKnockbackDir = Vec2(1.f, 0.f);  // 기본적으로 오른쪽
        }
        vKnockbackDir.Normalize();

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
    m_fKnockbackSpeed = 300.f;  // 넉백 초기 속도

    // 죽음 상태로 변경
    ChangeState(MONSTER_STATE::DAMAGE);
}

void CBasicMonster::UpdateDeathEffect()
{
    if (!m_bIsDying)
        return;

    float fDT = CTimeMgr::GetInst()->GetfDT();
    m_fDeathEffectTimer += fDT;

    // 넉백 이동 처리 (감속하면서)
    if (m_fKnockbackSpeed > 0.f)
    {
        // 감속 적용
        m_fKnockbackSpeed -= 800.f * fDT;  // 초당 800픽셀/초씩 감속
        if (m_fKnockbackSpeed < 0.f)
            m_fKnockbackSpeed = 0.f;

        // 넉백 이동
        if (GetRigidBody())
        {
            GetRigidBody()->SetVelocity(m_vKnockbackDir * m_fKnockbackSpeed);
        }
    }

    // 0.3초 후 실제로 죽음 처리
    if (m_fDeathEffectTimer >= 0.3f)
    {
        SetDead();
    }
}