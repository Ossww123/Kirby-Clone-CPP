#include "pch.h"
#include "CWaddleDoo.h"

CWaddleDoo::CWaddleDoo()
    : m_bBeamFired(false)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WADDLE_DOO);

    // 웨이들 두 전용 설정
    m_fSpeed = 60.f;                        // 웨이들 디보다 약간 느림
    SetAttackCooldown(4.f);                 // 4초 쿨타임
    m_fAttackRange = 200.f;                 // 200픽셀 범위
    m_fAttackReadyTime = 0.8f;              // 0.8초 준비
    m_fAttackDuration = 0.5f;               // 0.5초 공격

    // 애니메이션 생성
    CreateAnimations();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CWaddleDoo::~CWaddleDoo()
{
    // 상위 클래스에서 정리
}

void CWaddleDoo::Move()
{
    // 앞방에 벽이 있거나 바닥이 없으면 방향 전환
    if (CheckWallAhead() || !CheckGroundAhead())
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 계속 걷기
    MoveHorizontal(m_fSpeed);
}

void CWaddleDoo::Attack()
{
    // 빔 공격 실행
    if (!m_bBeamFired)
    {
        ShootBeam();
        m_bBeamFired = true;
    }
}

void CWaddleDoo::CreateAnimations()
{
    // 두 번째 행: 웨이들 두 - 시작 위치 (8, 40)
    Vec2 startPos = Vec2(8.f, 8.f + 32.f);

    // WALK 애니메이션 (1~8열)
    CreateBasicAnimation(L"WALK", startPos, 8, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // DAMAGE 애니메이션 (9~12열) - 빨아들임 중에도 사용
    CreateBasicAnimation(L"DAMAGE", Vec2(startPos.x + 32.f * 8, startPos.y), 4,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // ATTACK_READY 애니메이션 (13~14열)
    CreateBasicAnimation(L"ATTACK_READY", Vec2(startPos.x + 32.f * 12, startPos.y), 2,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // ATTACK 애니메이션 (15~17열)
    CreateBasicAnimation(L"ATTACK", Vec2(startPos.x + 32.f * 14, startPos.y), 3,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // IDLE 애니메이션
    CreateBasicAnimation(L"IDLE", startPos, 1, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.5f, true);
}

void CWaddleDoo::ShootBeam()
{
    // 빔 발사 로직
    CreateBeamProjectile();

    // TODO: 빔 발사 사운드
    // TODO: 빔 발사 이펙트
}

void CWaddleDoo::CreateBeamProjectile()
{
    // TODO: 빔 투사체 오브젝트 생성
    // CBeam* pBeam = new CBeam;
    // pBeam->SetPos(GetPos());
    // pBeam->SetDirection(GetPlayerDirection());
    // pBeam->SetSpeed(300.f);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pBeam, GROUP_TYPE::PROJECTILE);
}