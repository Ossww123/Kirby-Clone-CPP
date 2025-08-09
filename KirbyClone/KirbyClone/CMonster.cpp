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
    : CObject(OBJECT_TYPE::MONSTER_WADDLE_DEE)
    , m_pAnimator(nullptr)
    , m_pRigidBody(nullptr)
    , m_eCurState(MONSTER_STATE::IDLE)
    , m_ePrevState(MONSTER_STATE::END)
    , m_fSpeed(80.f)
    , m_iDir(1)
    , m_fStateTimer(0.f)
    , m_fIdleTime(1.f)
    , m_bHitWall(false)
    , m_fGroundCheckDist(32.f)
    , m_fWallCheckDist(32.f)
    , m_eMonsterType(OBJECT_TYPE::MONSTER_WADDLE_DEE)  // 새로 추가
{
    // 충돌체 생성 - 56x56으로 설정
    CreateCollider();
    GetCollider()->SetScale(Vec2(56.f, 56.f));

    // 애니메이터 생성
    CreateAnimator();
    m_pAnimator = GetAnimator();

    // 리지드바디 생성
    CreateRigidBody();
    m_pRigidBody = GetRigidBody();

    // 리지드바디 설정 (몬스터는 가벼움)
    m_pRigidBody->SetMass(0.8f);
    m_pRigidBody->SetMaxVelocity(200.f);
    m_pRigidBody->SetFriction(8.f);
    m_pRigidBody->SetUseGravity(true);

    // 애니메이션 생성
    CreateAnimation();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CMonster::~CMonster()
{
    m_pAnimator = nullptr;
    m_pRigidBody = nullptr;
}

void CMonster::SetMonsterType(OBJECT_TYPE _eType)
{
    m_eMonsterType = _eType;
    SetType(_eType);

    // 몬스터 타입별 기본 설정
    switch (_eType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        m_fSpeed = 80.f;
        break;
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        m_fSpeed = 60.f;
        break;
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        m_fSpeed = 100.f;
        m_pRigidBody->SetUseGravity(false);  // 날아다님
        break;
    case OBJECT_TYPE::MONSTER_GORDOS:
        m_fSpeed = 120.f;
        m_pRigidBody->SetUseGravity(false);  // 날아다님
        break;
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        m_fSpeed = 90.f;
        break;
    case OBJECT_TYPE::MONSTER_SPARKY:
        m_fSpeed = 70.f;
        break;
    }

    // 애니메이션 재생성
    CreateAnimation();
}

void CMonster::CreateAnimation()
{
    // 통합 enemies.bmp 스프라이트 시트 로드
    CTexture* pTex = CResMgr::GetInst()->LoadTexture(L"EnemiesSprite", L"texture\\enemy\\enemies.bmp");

    // 기본 프레임 크기와 간격 (1~6행: 32x32, 간격 32)
    Vec2 frameSize = Vec2(32.f, 32.f);
    Vec2 frameOffset = Vec2(32.f, 32.f);  // 간격도 32
    float frameDuration = 0.15f;

    switch (m_eMonsterType)
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        CreateWaddleDeeAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        CreateWaddleDooAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        CreateBrontoBurtAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    case OBJECT_TYPE::MONSTER_GORDOS:
        CreateGordosAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        CreateHotHeadAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    case OBJECT_TYPE::MONSTER_SPARKY:
        CreateSparkyAnimations(pTex, frameSize, frameOffset, frameDuration);
        break;
    }

    // 기본 애니메이션 재생
    m_pAnimator->Play(L"WALK", true);
}

void CMonster::CreateWaddleDeeAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 첫 번째 행: 웨이들 디 - 시작 위치 (8, 8)
    Vec2 startPos = Vec2(8.f, 8.f);

    // WALK 상태 (1~8열)
    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,                   // 시작 위치 (8, 8)
        frameSize,                  // 프레임 크기 32x32
        frameOffset,                // 다음 프레임 오프셋 (32, 0)
        frameDuration, 8, true);

    // DAMAGE 상태 (9~12열)
    m_pAnimator->CreateAnimation(L"DAMAGE", pTex,
        Vec2(startPos.x + frameOffset.x * 8, startPos.y),  // 9번째 열 위치
        frameSize,
        frameOffset,
        frameDuration, 4, false);

    // IDLE은 WALK의 첫 프레임 사용
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::CreateWaddleDooAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 두 번째 행: 웨이들 두 - 시작 위치 (8, 8 + 32)
    Vec2 startPos = Vec2(8.f, 8.f + 32.f);

    // WALK 상태 (1~8열)
    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 8, true);

    // DAMAGE 상태 (9~12열)
    m_pAnimator->CreateAnimation(L"DAMAGE", pTex,
        Vec2(startPos.x + frameOffset.x * 8, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 4, false);

    // ATTACK_READY 상태 (13~14열)
    m_pAnimator->CreateAnimation(L"ATTACK_READY", pTex,
        Vec2(startPos.x + frameOffset.x * 12, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 2, true);

    // ATTACK 상태 (15~17열)
    m_pAnimator->CreateAnimation(L"ATTACK", pTex,
        Vec2(startPos.x + frameOffset.x * 14, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 3, false);

    // IDLE
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::CreateBrontoBurtAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 세 번째 행: 브론토 버트 - 시작 위치 (8, 8 + 64)
    Vec2 startPos = Vec2(8.f, 8.f + 64.f);

    // FLY/WALK 상태 (1~4열)
    m_pAnimator->CreateAnimation(L"FLY", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 4, true);

    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 4, true);

    // DAMAGE 상태 (5~8열)
    m_pAnimator->CreateAnimation(L"DAMAGE", pTex,
        Vec2(startPos.x + frameOffset.x * 4, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 4, false);

    // IDLE
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::CreateGordosAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 네 번째 행: 고르도 - 시작 위치 (8, 8 + 96)
    Vec2 startPos = Vec2(8.f, 8.f + 96.f);

    // FLY/WALK 상태 (1~4열)
    m_pAnimator->CreateAnimation(L"FLY", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 4, true);

    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 4, true);

    // IDLE
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::CreateHotHeadAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 다섯 번째 행: 핫 헤드 - 시작 위치 (8, 8 + 128)
    Vec2 startPos = Vec2(8.f, 8.f + 128.f);

    // WALK 상태 (1~8열)
    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 8, true);

    // DAMAGE 상태 (9~12열)
    m_pAnimator->CreateAnimation(L"DAMAGE", pTex,
        Vec2(startPos.x + frameOffset.x * 8, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 4, false);

    // ATTACK_READY 상태 (13열)
    m_pAnimator->CreateAnimation(L"ATTACK_READY", pTex,
        Vec2(startPos.x + frameOffset.x * 12, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 1, true);

    // ATTACK 상태 (14~15열)
    m_pAnimator->CreateAnimation(L"ATTACK", pTex,
        Vec2(startPos.x + frameOffset.x * 13, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 2, false);

    // IDLE
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::CreateSparkyAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration)
{
    // 여섯 번째 행: 스파키 - 시작 위치 (8, 8 + 160)
    Vec2 startPos = Vec2(8.f, 8.f + 160.f);

    // WALK 상태 (1~5열)
    m_pAnimator->CreateAnimation(L"WALK", pTex,
        startPos,
        frameSize,
        frameOffset,
        frameDuration, 5, true);

    // DAMAGE 상태 (9~12열)
    m_pAnimator->CreateAnimation(L"DAMAGE", pTex,
        Vec2(startPos.x + frameOffset.x * 8, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 4, false);

    // ATTACK_READY 상태 (13~14열)
    m_pAnimator->CreateAnimation(L"ATTACK_READY", pTex,
        Vec2(startPos.x + frameOffset.x * 12, startPos.y),
        frameSize,
        frameOffset,
        frameDuration, 2, true);

    // ATTACK 상태 - 일곱 번째 행의 큰 스프라이트 사용
    // 일곱 번째 행: (8, 200), 크기 64x64, 간격 64
    Vec2 bigFrameSize = Vec2(64.f, 64.f);
    Vec2 bigFrameOffset = Vec2(64.f, 64.f);
    Vec2 attackStartPos = Vec2(8.f, 200.f);

    m_pAnimator->CreateAnimation(L"ATTACK", pTex,
        attackStartPos,
        bigFrameSize,
        bigFrameOffset,
        frameDuration, 3, false);  // 스파키 공격 프레임 수 추정

    // IDLE
    m_pAnimator->CreateAnimation(L"IDLE", pTex,
        startPos,
        frameSize,
        frameOffset,
        0.5f, 1, true);
}

void CMonster::Update()
{
    UpdateState();
    UpdateMove();

    // 리지드바디 업데이트
    if (nullptr != m_pRigidBody)
        m_pRigidBody->Update();

    // 애니메이터 업데이트
    if (nullptr != m_pAnimator)
        m_pAnimator->Update();

    // 상태 타이머 업데이트
    m_fStateTimer += CTimeMgr::GetInst()->GetfDT();
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

void CMonster::UpdateIdle()
{
    // 일정 시간 정지 후 걷기 시작
    if (m_fStateTimer >= m_fIdleTime)
    {
        if (m_eMonsterType == OBJECT_TYPE::MONSTER_BRONTO_BURT ||
            m_eMonsterType == OBJECT_TYPE::MONSTER_GORDOS)
        {
            ChangeState(MONSTER_STATE::FLY);
        }
        else
        {
            ChangeState(MONSTER_STATE::WALK);
        }
    }
}

void CMonster::UpdateWalk()
{
    // 앞방에 벽이 있거나 바닥이 없으면 방향 전환
    if (CheckWallAhead() || !CheckGroundAhead())
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 계속 걷기
    m_pRigidBody->SetVelocityX(m_fSpeed * m_iDir);
}

void CMonster::UpdateFly()
{
    // 비행형 몬스터 (브론토 버트, 고르도)
    // 간단한 좌우 이동
    m_pRigidBody->SetVelocityX(m_fSpeed * m_iDir);

    // 화면 경계나 일정 시간 후 방향 전환
    if (m_fStateTimer >= 3.0f)
    {
        TurnAround();
        m_fStateTimer = 0.f;
    }
}

void CMonster::UpdateTurn()
{
    // 방향 전환 시간 (0.2초)
    if (m_fStateTimer >= 0.2f)
    {
        TurnAround();

        if (m_eMonsterType == OBJECT_TYPE::MONSTER_BRONTO_BURT ||
            m_eMonsterType == OBJECT_TYPE::MONSTER_GORDOS)
        {
            ChangeState(MONSTER_STATE::FLY);
        }
        else
        {
            ChangeState(MONSTER_STATE::WALK);
        }
    }
    else
    {
        // 방향 전환 중에는 멈춤
        m_pRigidBody->SetVelocityX(0.f);
    }
}

void CMonster::UpdateDamage()
{
    // 데미지 상태 일정 시간 유지
    if (m_fStateTimer >= 0.5f)
    {
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateAttackReady()
{
    // 공격 준비 상태 (웨이들 두, 핫 헤드, 스파키)
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::ATTACK);
    }
}

void CMonster::UpdateAttack()
{
    // 공격 애니메이션 종료 후 걷기 상태로
    if (m_fStateTimer >= 1.0f)
    {
        ChangeState(MONSTER_STATE::WALK);
    }
}

void CMonster::UpdateMove()
{
    // 바닥에 있을 때만 이동 로직 적용 (비행형 제외)
    if (m_eMonsterType != OBJECT_TYPE::MONSTER_BRONTO_BURT &&
        m_eMonsterType != OBJECT_TYPE::MONSTER_GORDOS)
    {
        if (m_pRigidBody && m_pRigidBody->IsGround())
        {
            // 상태별 이동은 UpdateState에서 처리됨
        }
    }
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
        m_pAnimator->Play(L"IDLE", true);
        break;
    case MONSTER_STATE::WALK:
        m_pAnimator->Play(L"WALK", true);
        break;
    case MONSTER_STATE::FLY:
        m_pAnimator->Play(L"FLY", true);
        break;
    case MONSTER_STATE::TURN:
        m_pAnimator->Play(L"WALK", true);
        break;
    case MONSTER_STATE::DAMAGE:
        m_pAnimator->Play(L"DAMAGE", false);
        break;
    case MONSTER_STATE::ATTACK_READY:
        m_pAnimator->Play(L"ATTACK_READY", true);
        break;
    case MONSTER_STATE::ATTACK:
        m_pAnimator->Play(L"ATTACK", false);
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

bool CMonster::CheckWallAhead()
{
    // 간단한 벽 체크 (실제 구현 필요)
    return false;
}

bool CMonster::CheckGroundAhead()
{
    // 간단한 바닥 체크 (실제 구현 필요)
    return true;
}