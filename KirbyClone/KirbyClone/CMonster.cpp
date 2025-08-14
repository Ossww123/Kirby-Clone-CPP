#include "pch.h"
#include "CMonster.h"

#include "CTimeMgr.h"
#include "CCollider.h"
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CCamera.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CTile.h"

CMonster::CMonster()
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE)  // 기본값, 자식에서 변경
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fStateTimer(0.f)
    , m_fSpeed(DEFAULT_SPEED)
    , m_iDir(1)
    , m_fIdleTime(DEFAULT_IDLE_TIME)
    , m_bHitWall(false)
    , m_fGroundCheckDist(32.f)
    , m_fWallCheckDist(32.f)
    , m_pEnemyTex(nullptr)
{
    // 기본 컴포넌트 생성
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    CreateAnimator();
    CreateRigidBody();

    // 리지드바디 기본 설정
    GetRigidBody()->SetMass(0.8f);
    GetRigidBody()->SetMaxVelocity(200.f);
    GetRigidBody()->SetFriction(8.f);
    GetRigidBody()->SetUseGravity(true);

    // 공통 텍스처 로드
    LoadEnemySpriteSheet();

    // 자식 클래스에서 CreateAnimations() 호출됨
    // 초기 상태는 자식 클래스에서 설정
}

CMonster::~CMonster()
{
    // 상위 클래스 CObject에서 컴포넌트 해제 처리
    // m_pEnemyTex는 리소스 매니저에서 관리하므로 delete 하지 않음
}

void CMonster::Update()
{
    // 상태 업데이트
    UpdateState();

    // 이동 업데이트
    UpdateMove();

    // 컴포넌트 업데이트
    if (nullptr != GetRigidBody())
        GetRigidBody()->Update();

    if (nullptr != GetAnimator())
        GetAnimator()->Update();

    // 상태 타이머 업데이트
    m_fStateTimer += CTimeMgr::GetInst()->GetfDT();
}

void CMonster::ChangeState(MONSTER_STATE _eState)
{
    m_ePrevState = m_eCurState;
    m_eCurState = _eState;
    m_fStateTimer = 0.f;

    // 상태별 애니메이션 재생
    switch (_eState)
    {
    case MONSTER_STATE::IDLE:
        GetAnimator()->Play(L"IDLE", true);
        break;
    case MONSTER_STATE::WALK:
        GetAnimator()->Play(L"WALK", true);
        break;
    case MONSTER_STATE::FLY:
        GetAnimator()->Play(L"FLY", true);
        break;
    case MONSTER_STATE::TURN:
        GetAnimator()->Play(L"WALK", true);
        break;
    case MONSTER_STATE::DAMAGE:
        GetAnimator()->Play(L"DAMAGE", false);
        break;
    case MONSTER_STATE::ATTACK_READY:
        GetAnimator()->Play(L"ATTACK_READY", true);
        break;
    case MONSTER_STATE::ATTACK:
        GetAnimator()->Play(L"ATTACK", false);
        break;
    }
}

void CMonster::TakeDamage()
{
    ChangeState(MONSTER_STATE::DAMAGE);
}

void CMonster::TurnAround()
{
    m_iDir *= -1;
}

void CMonster::MoveHorizontal(float speed)
{
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(speed * m_iDir);
    }
}

void CMonster::HandleWallCollision()
{
    // 벽 충돌 시 방향 전환
    if (CheckWallAhead())
    {
        TurnAround();
        m_bHitWall = true;
    }
    else
    {
        m_bHitWall = false;
    }
}

void CMonster::LoadEnemySpriteSheet()
{
    if (nullptr == m_pEnemyTex)
    {
        m_pEnemyTex = CResMgr::GetInst()->LoadTexture(L"EnemiesSprite", L"texture\\enemy\\enemies.bmp");
    }
}

void CMonster::CreateBasicAnimation(const wstring& name, Vec2 startPos, int frameCount,
    Vec2 frameSize, Vec2 frameOffset, float duration, bool loop)
{
    if (nullptr != GetAnimator() && nullptr != m_pEnemyTex)
    {
        GetAnimator()->CreateAnimation(name, m_pEnemyTex, startPos, frameSize,
            frameOffset, duration, frameCount, loop);
    }
}

void CMonster::UpdateState()
{
    switch (m_eCurState)
    {
    case MONSTER_STATE::IDLE:
        UpdateIdle();
        break;
    case MONSTER_STATE::WALK:
        UpdateWalk();
        break;
    case MONSTER_STATE::FLY:
        UpdateFly();
        break;
    case MONSTER_STATE::TURN:
        UpdateTurn();
        break;
    case MONSTER_STATE::DAMAGE:
        UpdateDamage();
        break;
    case MONSTER_STATE::ATTACK_READY:
        UpdateAttackReady();
        break;
    case MONSTER_STATE::ATTACK:
        UpdateAttack();
        break;
    }
}

void CMonster::UpdateMove()
{
    // 자식 클래스에서 Move() 호출로 실제 이동 처리
    // 이 함수는 Move() 호출 후 추가 처리가 필요할 때 사용
}

void CMonster::UpdateIdle()
{
    // 일정 시간 정지 후 다음 행동 결정
    if (m_fStateTimer >= m_fIdleTime)
    {
        // 기본적으로 걷기 상태로 전환 (자식에서 override 가능)
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateWalk()
{
    // 자식 클래스의 Move() 함수 호출
    Move();
}

void CMonster::UpdateFly()
{
    // 기본 비행 로직 (자식에서 override 권장)
    Move();

    // 일정 시간 후 방향 전환
    if (m_fStateTimer >= 3.0f)
    {
        TurnAround();
        m_fStateTimer = 0.f;
    }
}

void CMonster::UpdateTurn()
{
    // 방향 전환 시간
    if (m_fStateTimer >= TURN_DURATION)
    {
        TurnAround();
        ChangeState(MONSTER_STATE::WALK);  // 기본적으로 걷기로 복귀
    }
    else
    {
        // 방향 전환 중에는 멈춤
        if (nullptr != GetRigidBody())
        {
            GetRigidBody()->SetVelocityX(0.f);
        }
    }
}

void CMonster::UpdateDamage()
{
    // 데미지 상태 지속 시간
    if (m_fStateTimer >= DAMAGE_DURATION)
    {
        ChangeState(MONSTER_STATE::WALK);
    }

    // 데미지 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

void CMonster::UpdateAttackReady()
{
    // 기본 공격 준비 시간 (자식에서 override 권장)
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }

    // 공격 준비 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

void CMonster::UpdateAttack()
{
    // 기본 공격 지속 시간 (자식에서 override 권장)
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::WALK);
    }

    // 공격 중에는 이동 정지
    if (nullptr != GetRigidBody())
    {
        GetRigidBody()->SetVelocityX(0.f);
    }
}

bool CMonster::CheckWallAhead()
{
    // TODO: 실제 타일맵 또는 충돌체를 이용한 벽 체크 구현
    // 현재는 임시로 false 반환

    // 예시 구현 (실제로는 레이캐스팅이나 충돌 검사 필요)
    Vec2 currentPos = GetPos();
    Vec2 checkPos = Vec2(currentPos.x + (m_fWallCheckDist * m_iDir), currentPos.y);

    // 화면 경계 체크 (임시)
    if (checkPos.x < 0 || checkPos.x > 1920)  // 화면 너비 가정
    {
        return true;
    }

    return false;
}

bool CMonster::CheckGroundAhead()
{
    // TODO: 실제 타일맵 또는 충돌체를 이용한 바닥 체크 구현
    // 현재는 임시로 true 반환

    // 예시 구현 (실제로는 레이캐스팅이나 충돌 검사 필요)
    Vec2 currentPos = GetPos();
    Vec2 checkPos = Vec2(currentPos.x + (m_fWallCheckDist * m_iDir),
        currentPos.y + m_fGroundCheckDist);

    // 화면 하단 경계 체크 (임시)
    if (checkPos.y > 1080)  // 화면 높이 가정
    {
        return false;
    }

    return true;
}