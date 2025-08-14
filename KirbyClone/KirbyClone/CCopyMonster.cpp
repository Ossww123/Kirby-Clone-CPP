#include "pch.h"
#include "CCopyMonster.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CSceneMgr.h"
#include "CScene.h"
// #include "CPlayer.h"  // TODO: 플레이어 클래스 참조 필요

CCopyMonster::CCopyMonster()
    : CBasicMonster()
    , m_bAttacking(false)
    , m_fAttackCooldown(0.f)
    , m_fMaxAttackCooldown(3.f)         // 기본 3초 쿨타임
    , m_fAttackRange(150.f)             // 기본 150픽셀 범위
    , m_fAttackReadyTime(1.f)           // 1초 준비 시간
    , m_fAttackDuration(1.f)            // 1초 공격 지속
    , m_vPlayerPos(Vec2(0.f, 0.f))
    , m_bPlayerDetected(false)
{
    // 카피 능력 몬스터는 공격 가능하므로 특별한 설정 불필요
}

CCopyMonster::~CCopyMonster()
{
    // 상위 클래스에서 정리 처리
}

bool CCopyMonster::CanAttack() const
{
    return m_fAttackCooldown <= 0.f && !m_bBeingInhaled && IsPlayerInRange();
}

void CCopyMonster::StartAttack()
{
    if (!CanAttack())
        return;

    m_bAttacking = true;
    ChangeState(MONSTER_STATE::ATTACK_READY);
}

void CCopyMonster::EndAttack()
{
    m_bAttacking = false;
    m_fAttackCooldown = m_fMaxAttackCooldown;  // 쿨타임 시작
    ChangeState(MONSTER_STATE::WALK);
}

void CCopyMonster::OnCopyAbilityGiven()
{
    // 카피 능력 제공 시 기본 처리
    // 자식 클래스에서 override하여 추가 처리 가능

    // TODO: 커비에게 능력 부여
    COPY_ABILITY ability = GetCopyAbility();

    // TODO: 이펙트 생성
    // TODO: 사운드 재생

    // 몬스터 제거
    SetDead();
}

void CCopyMonster::UpdateAttackCooldown()
{
    if (m_fAttackCooldown > 0.f)
    {
        m_fAttackCooldown -= CTimeMgr::GetInst()->GetfDT();
        if (m_fAttackCooldown < 0.f)
            m_fAttackCooldown = 0.f;
    }
}

void CCopyMonster::CheckAttackCondition()
{
    // 플레이어 위치 업데이트 및 공격 조건 체크
    // TODO: 실제 플레이어 위치 가져오기
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // CPlayer* pPlayer = pCurScene->FindPlayer();
    // if (pPlayer) {
    //     m_vPlayerPos = pPlayer->GetPos();
    //     m_bPlayerDetected = IsPlayerInRange();
    // }

    // 공격 가능하면 공격 시작
    if (CanAttack() && m_bPlayerDetected)
    {
        StartAttack();
    }
}

void CCopyMonster::UpdateWalk()
{
    // 공격 쿨타임 업데이트
    UpdateAttackCooldown();

    // 공격 조건 체크
    CheckAttackCondition();

    // 공격 중이 아니면 일반 걷기
    if (!m_bAttacking)
    {
        CBasicMonster::UpdateWalk();
    }
}

void CCopyMonster::UpdateAttackReady()
{
    // 공격 준비 상태 처리
    if (m_fStateTimer >= m_fAttackReadyTime)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }
    else
    {
        // 공격 준비 중에는 플레이어 조준
        AimAtPlayer();

        // 이동 정지
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
}

void CCopyMonster::UpdateAttack()
{
    // 공격 실행
    if (m_fStateTimer < m_fAttackDuration)
    {
        // 자식 클래스의 Attack() 함수 호출
        Attack();

        // 공격 중에는 이동 정지
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
    else
    {
        // 공격 종료
        EndAttack();
    }
}

bool CCopyMonster::IsPlayerInRange() const
{
    Vec2 vDist = m_vPlayerPos - GetPos();
    float fDistance = vDist.Length();
    return fDistance <= m_fAttackRange;
}

Vec2 CCopyMonster::GetPlayerDirection() const
{
    Vec2 vDirection = m_vPlayerPos - GetPos();
    if (vDirection.Length() > 0.1f)
    {
        vDirection.Normalize();
    }
    return vDirection;
}

void CCopyMonster::AimAtPlayer()
{
    // 플레이어 방향으로 몸 돌리기
    Vec2 vPlayerDir = GetPlayerDirection();
    if (vPlayerDir.x > 0.f && m_iDir < 0)
    {
        TurnAround();
    }
    else if (vPlayerDir.x < 0.f && m_iDir > 0)
    {
        TurnAround();
    }
}

void CCopyMonster::ProcessAttackLogic()
{
    // 공격 로직 처리 - 자식 클래스에서 Attack() 구현
    // 이 함수는 필요시 자식에서 override
}