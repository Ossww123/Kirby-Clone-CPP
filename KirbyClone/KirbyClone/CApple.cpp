#include "gamePCH.h"
#include "CApple.h"

#include "CTimeMgr.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CCollider.h"
#include "CRigidBody.h"
#include "CPlayer.h"
#include "CEventMgr.h"

CApple::CApple()
    : m_eAppleState(APPLE_STATE::WARNING)
    , m_bGravityEnabled(false)
    , m_fFallSpeed(0.f)
    , m_fGravityAccel(800.f)
    , m_fMaxFallSpeed(400.f)
    , m_fWarningDuration(1.0f)
    , m_fWarningTimer(0.f)
    , m_fRollSpeed(150.f)
    , m_iRollDirection(0)
    , m_bRollingStarted(false)
    , m_fLifetime(10.f)
    , m_fLifetimeTimer(0.f)
    , m_fBounceSpeed(300.f)
    , m_fBounceHorizontalSpeed(0.f)
    , m_bHasBounced(false)
    , m_bHitPlayer(false)
    , m_bHitGround(false)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WADDLE_DEE);
    
    // 사과 크기 설정
    SetScale(Vec2(32.f, 32.f));
    
    // 충돌체 설정
    CreateCollider();
    GetCollider()->SetScale(Vec2(24.f, 24.f));
    
    // 물리체 설정 (중력 적용용)
    CreateRigidBody();
    GetRigidBody()->SetMass(1.f);
    GetRigidBody()->SetMaxVelocity(m_fMaxFallSpeed);
    
    // 초기 상태
    ChangeState(MONSTER_STATE::IDLE);
    
    // 애니메이션 로드
    LoadAnimationsFromFile(L"apple_animations.json");
}

CApple::~CApple()
{
}

void CApple::Move()
{
    // 상태별 업데이트
    switch (m_eAppleState)
    {
    case APPLE_STATE::WARNING:
        UpdateWarning();
        break;
    case APPLE_STATE::FALLING:
        UpdateFalling();
        break;
    case APPLE_STATE::BOUNCING:
        UpdateBouncing();
        break;
    case APPLE_STATE::ROLLING:
        UpdateRolling();
        break;
    }
    
    // 생존 시간 체크
    m_fLifetimeTimer += CTimeMgr::GetInst()->GetDT();
    if (m_fLifetimeTimer >= m_fLifetime)
    {
        // 수명이 다하면 넉백-사라짐 이펙트 적용
        m_vDamageSourcePos = Vec2(0.f, 0.f); // 기본 데미지 소스
        TakeDamage();
    }
}

void CApple::UpdateWarning()
{
    float fDT = CTimeMgr::GetInst()->GetDT();
    m_fWarningTimer += fDT;
    
    // 전조 시간이 끝나면 낙하 시작
    if (m_fWarningTimer >= m_fWarningDuration)
    {
        m_eAppleState = APPLE_STATE::FALLING;
        m_bGravityEnabled = true;
        ChangeState(MONSTER_STATE::IDLE);  // 낙하 중 애니메이션
    }
}

void CApple::UpdateFalling()
{
    if (!m_bGravityEnabled || m_bHitGround)
        return;
        
    float fDT = CTimeMgr::GetInst()->GetDT();
    
    // 중력 가속도 적용
    m_fFallSpeed += m_fGravityAccel * fDT;
    
    // 최대 낙하 속도 제한
    if (m_fFallSpeed > m_fMaxFallSpeed)
        m_fFallSpeed = m_fMaxFallSpeed;
    
    // RigidBody를 통한 이동
    if (GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2(0.f, m_fFallSpeed));
    }
    else
    {
        // RigidBody가 없는 경우 직접 위치 변경
        Vec2 vPos = GetPos();
        vPos.y += m_fFallSpeed * fDT;
        SetPos(vPos);
    }
}

void CApple::UpdateBouncing()
{
    float fDT = CTimeMgr::GetInst()->GetDT();
    
    // 바운스 상태에서는 위쪽으로 속도가 있고, 중력이 적용됨
    m_fFallSpeed += m_fGravityAccel * fDT; // 중력으로 떨어짐
    
    // RigidBody를 통한 이동 (포물선 운동)
    if (GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2(m_fBounceHorizontalSpeed, m_fFallSpeed));
    }
    else
    {
        // RigidBody가 없는 경우 직접 위치 변경
        Vec2 vPos = GetPos();
        vPos.x += m_fBounceHorizontalSpeed * fDT; // 수평 이동 추가
        vPos.y += m_fFallSpeed * fDT;
        SetPos(vPos);
    }
    
    // 바운스가 끝났는지 체크 (다시 떨어지기 시작하면)
    if (m_fFallSpeed > 0 && m_eAppleState == APPLE_STATE::BOUNCING)
    {
        // 바운스 상태 유지하되 다음 충돌을 기다림
    }
}

void CApple::UpdateRolling()
{
    if (!m_bRollingStarted || m_iRollDirection == 0)
        return;
        
    float fDT = CTimeMgr::GetInst()->GetDT();
    
    // 굴러가는 방향으로 이동
    Vec2 vPos = GetPos();
    vPos.x += m_iRollDirection * m_fRollSpeed * fDT;
    SetPos(vPos);
    
    // RigidBody가 있다면 속도 설정
    if (GetRigidBody())
    {
        GetRigidBody()->SetVelocity(Vec2((float)m_iRollDirection * m_fRollSpeed, 0.f));
    }
}

void CApple::OnCollisionEnter(CCollider* _pOther)
{
    CObject* pOtherObj = _pOther->GetOwner();
    OBJECT_TYPE eOtherType = pOtherObj->GetType();
    
    // 플레이어와 충돌
    if (eOtherType == OBJECT_TYPE::PLAYER && !m_bHitPlayer)
    {
        HandlePlayerHit();
    }
    // 타일과 충돌 (바닥)
    else if (eOtherType >= OBJECT_TYPE::TILE_GROUND && eOtherType <= OBJECT_TYPE::TILE_INVISIBLE)
    {
        CheckGroundCollision();
    }
}

void CApple::OnCollision(CCollider* _pOther)
{
    // 지속적인 충돌 처리 (필요시)
}

void CApple::HandlePlayerHit()
{
    m_bHitPlayer = true;
    
    // 플레이어 위치를 데미지 소스로 설정
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (pCurScene)
    {
        const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
        if (!vecPlayers.empty())
        {
            CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
            m_vDamageSourcePos = pPlayer->GetPos();
        }
    }
    
    // 사과는 플레이어와 충돌 시 넉백-사라짐 이펙트 적용
    TakeDamage();
}

void CApple::CheckGroundCollision()
{
    // 낙하 중이거나 바운스 중일 때 바닥과 충돌
    if ((m_eAppleState == APPLE_STATE::FALLING || m_eAppleState == APPLE_STATE::BOUNCING) && m_fFallSpeed > 0)
    {
        if (!m_bHasBounced)
        {
            // 첫 번째 바닥 충돌 - 바운스
            m_bHasBounced = true;
            m_eAppleState = APPLE_STATE::BOUNCING;
            m_fFallSpeed = -m_fBounceSpeed; // 위로 튀어오르도록 음수 설정
            
            // 바운스 시 수평 속도 설정 (랜덤 방향으로)
            // 플레이어 위치에 따라 방향 결정
            CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
            if (pCurScene)
            {
                const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
                if (!vecPlayers.empty())
                {
                    CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
                    Vec2 playerPos = pPlayer->GetPos();
                    Vec2 applePos = GetPos();
                    
                    // 플레이어 쪽으로 약간 튀어가도록 설정 (50-100 속도로)
                    float bounceDirection = (playerPos.x > applePos.x) ? 1.0f : -1.0f;
                    m_fBounceHorizontalSpeed = bounceDirection * (75.f + (rand() % 50)); // 75-125 속도
                }
                else
                {
                    // 플레이어가 없으면 랜덤 방향
                    m_fBounceHorizontalSpeed = ((rand() % 2) ? 1.0f : -1.0f) * 100.f;
                }
            }
            
            // RigidBody에 바운스 속도 적용
            if (GetRigidBody())
            {
                GetRigidBody()->SetVelocity(Vec2(m_fBounceHorizontalSpeed, -m_fBounceSpeed));
            }
        }
        else
        {
            // 두 번째 바닥 충돌 - 굴러가기 시작
            m_bHitGround = true;
            m_fFallSpeed = 0.f;
            m_fBounceHorizontalSpeed = 0.f; // 수평 속도 제거
            
            // RigidBody 정지
            if (GetRigidBody())
            {
                GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
            }
            
            // 굴러가기 시작
            StartRolling();
            
            // 바닥에 떨어진 후 5초 후 자동 삭제 (굴러가는 시간 고려)
            m_fLifetime = 5.f;
            m_fLifetimeTimer = 0.f;
        }
    }
}

void CApple::StartRolling()
{
    // 플레이어 위치를 찾아서 굴러갈 방향 결정
    CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    if (!pCurScene)
        return;
        
    const vector<CObject*>& vecPlayers = pCurScene->GetGroupObject(GROUP_TYPE::PLAYER);
    if (vecPlayers.empty())
        return;
        
    CPlayer* pPlayer = (CPlayer*)vecPlayers[0];
    Vec2 playerPos = pPlayer->GetPos();
    Vec2 applePos = GetPos();
    
    // 플레이어가 사과보다 왼쪽에 있으면 왼쪽으로, 오른쪽에 있으면 오른쪽으로 굴러가기
    if (playerPos.x < applePos.x)
    {
        m_iRollDirection = -1;  // 왼쪽으로
        ChangeState(MONSTER_STATE::WALK);  // COUNTER_CLOCKWISE_ROTATE 애니메이션용
    }
    else
    {
        m_iRollDirection = 1;   // 오른쪽으로
        ChangeState(MONSTER_STATE::DAMAGE); // CLOCKWISE_ROTATE 애니메이션용
    }
    
    m_eAppleState = APPLE_STATE::ROLLING;
    m_bRollingStarted = true;
}

void CApple::StartWarningPhase(float _fDuration)
{
    m_eAppleState = APPLE_STATE::WARNING;
    m_fWarningDuration = _fDuration;
    m_fWarningTimer = 0.f;
    m_bGravityEnabled = false;
    ChangeState(MONSTER_STATE::IDLE);  // WARNING 동안 IDLE 애니메이션 사용
}

void CApple::SetupAnimationMapping()
{
    // 사과 애니메이션 매핑 - 굴러가는 경우 제외하고 모든 상태에서 IDLE 사용
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";                    // 전조 및 기본 상태
    m_mapStateToAnimation[MONSTER_STATE::WALK] = L"COUNTER_CLOCKWISE_ROTATE"; // 왼쪽으로 굴러가기
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"CLOCKWISE_ROTATE";      // 오른쪽으로 굴러가기 
    m_mapStateToAnimation[MONSTER_STATE::ATTACK] = L"IDLE";                  // 공격 시 IDLE 사용
    m_mapStateToAnimation[MONSTER_STATE::BEING_INHALED] = L"IDLE";           // 흡입 시 IDLE 사용
    m_mapStateToAnimation[MONSTER_STATE::FLY] = L"IDLE";                     // 낙하 시 IDLE 사용
    m_mapStateToAnimation[MONSTER_STATE::TURN] = L"IDLE";                    // 전환 시 IDLE 사용
}