#include "pch.h"
#include "CSparky.h"

CSparky::CSparky()
    : m_bJumping(false)
    , m_fJumpTimer(0.f)
    , m_fJumpInterval(2.f)                  // 2초마다 점프
    , m_fJumpForce(250.f)                   // 점프 힘
    , m_bElectricFieldCreated(false)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_SPARKY);

    // 스파키 전용 설정
    m_fSpeed = 70.f;                        // 느린 이동 (점프로 보완)
    SetAttackCooldown(5.f);                 // 5초 쿨타임 (강력한 공격)
    m_fAttackRange = 120.f;                 // 120픽셀 범위
    m_fAttackReadyTime = 1.2f;              // 1.2초 준비 (긴 준비시간)
    m_fAttackDuration = 1.5f;               // 1.5초 공격 (긴 지속시간)

    // 애니메이션 생성
    CreateAnimations();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CSparky::~CSparky()
{
    // 상위 클래스에서 정리
}

void CSparky::Move()
{
    // 점프 패턴 업데이트
    UpdateJumpPattern();

    // 앞방에 벽이 있으면 방향 전환
    if (CheckWallAhead())
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 점프 이동
    if (!m_bJumping)
    {
        JumpMove();
    }
}

void CSparky::Attack()
{
    // 전기 공격 실행
    if (!m_bElectricFieldCreated)
    {
        CreateElectricField();
        m_bElectricFieldCreated = true;
    }

    // 공격 중 지속적으로 스파크 생성
    if ((int)(m_fStateTimer * 4) % 2 == 0)  // 0.25초마다
    {
        CreateElectricSpark();
    }
}

void CSparky::UpdateWalk()
{
    // 점프 타이머 업데이트
    m_fJumpTimer += CTimeMgr::GetInst()->GetfDT();

    // 부모 클래스의 UpdateWalk 호출
    CCopyMonster::UpdateWalk();
}

void CSparky::CreateAnimations()
{
    // 여섯 번째 행: 스파키 - 시작 위치 (8, 168)
    Vec2 startPos = Vec2(8.f, 8.f + 160.f);

    // WALK 애니메이션 (1~5열) - 점프 애니메이션
    CreateBasicAnimation(L"WALK", startPos, 5, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // DAMAGE 애니메이션 (9~12열)
    CreateBasicAnimation(L"DAMAGE", Vec2(startPos.x + 32.f * 8, startPos.y), 4,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // ATTACK_READY 애니메이션 (13~14열)
    CreateBasicAnimation(L"ATTACK_READY", Vec2(startPos.x + 32.f * 12, startPos.y), 2,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // ATTACK 애니메이션 - 일곱 번째 행의 큰 스프라이트 사용
    Vec2 bigFrameSize = Vec2(64.f, 64.f);
    Vec2 attackStartPos = Vec2(8.f, 200.f);
    CreateBasicAnimation(L"ATTACK", attackStartPos, 3, bigFrameSize, Vec2(64.f, 64.f), 0.15f, false);

    // IDLE 애니메이션
    CreateBasicAnimation(L"IDLE", startPos, 1, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.5f, true);
}

void CSparky::JumpMove()
{
    if (m_fJumpTimer >= m_fJumpInterval && nullptr != GetRigidBody())
    {
        // 점프 실행
        if (GetRigidBody()->IsGround())  // 바닥에 있을 때만 점프
        {
            GetRigidBody()->AddForce(Vec2(m_fSpeed * m_iDir, -m_fJumpForce));
            m_bJumping = true;
            m_fJumpTimer = 0.f;
        }
    }
    else if (m_bJumping && GetRigidBody()->IsGround())
    {
        // 착지 시 점프 상태 해제
        m_bJumping = false;
    }
    else if (!m_bJumping)
    {
        // 일반 이동
        MoveHorizontal(m_fSpeed * 0.5f);  // 점프 없을 때는 천천히
    }
}

void CSparky::UpdateJumpPattern()
{
    // 점프 패턴 관리 로직
    // 현재는 일정 간격으로 점프하지만, 나중에 더 복잡한 패턴 추가 가능

    // 리지드바디 상태 확인
    if (nullptr != GetRigidBody())
    {
        // 공중에 있을 때 수평 이동 제어
        if (!GetRigidBody()->IsGround() && m_bJumping)
        {
            // 공중에서 약간의 수평 제어
            GetRigidBody()->AddForce(Vec2(m_fSpeed * m_iDir * 0.1f, 0.f));
        }
    }
}

void CSparky::CreateElectricField()
{
    // 전기장 생성 로직
    Vec2 currentPos = GetPos();

    // TODO: 전기장 이펙트 오브젝트 생성
    // CElectricField* pField = new CElectricField;
    // pField->SetPos(currentPos);
    // pField->SetRange(m_fAttackRange);
    // pField->SetDuration(m_fAttackDuration);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pField, GROUP_TYPE::EFFECT);

    // TODO: 전기장 사운드
    // TODO: 화면 진동 효과
}

void CSparky::CreateElectricSpark()
{
    // 전기 스파크 생성 로직
    Vec2 currentPos = GetPos();

    // 랜덤한 위치에 스파크 생성
    for (int i = 0; i < 3; ++i)
    {
        float fRandomX = (rand() % 100 - 50) * 0.01f * m_fAttackRange;  // -범위/2 ~ +범위/2
        float fRandomY = (rand() % 60 - 30) * 0.01f * m_fAttackRange;   // -범위/3 ~ +범위/3

        Vec2 sparkPos = currentPos + Vec2(fRandomX, fRandomY);

        // TODO: 스파크 이펙트 오브젝트 생성
        // CElectricSpark* pSpark = new CElectricSpark;
        // pSpark->SetPos(sparkPos);
        // pSpark->SetLifetime(0.3f);
        // 
        // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
        // pCurScene->AddObject(pSpark, GROUP_TYPE::EFFECT);
    }
}