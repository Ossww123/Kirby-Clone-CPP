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