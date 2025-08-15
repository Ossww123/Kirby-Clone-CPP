#include "pch.h"
#include "CHotHead.h"

CHotHead::CHotHead()
    : m_bFireSpat(false)
    , m_iFireCount(0)
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_HOT_HEAD);

    // 핫 헤드 전용 설정
    m_fSpeed = 90.f;                        // 빠른 이동
    SetAttackCooldown(3.5f);                // 3.5초 쿨타임
    m_fAttackRange = 180.f;                 // 180픽셀 범위
    m_fAttackReadyTime = 0.6f;              // 0.6초 준비
    m_fAttackDuration = 0.8f;               // 0.8초 공격

    // 애니메이션 생성

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CHotHead::~CHotHead()
{
    // 상위 클래스에서 정리
}

void CHotHead::Move()
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

void CHotHead::Attack()
{
    // 화염 공격 실행 (연속 발사)
    if (!m_bFireSpat)
    {
        SpitFire();
        m_bFireSpat = true;
        m_iFireCount = 0;
    }

    // 0.2초마다 화염 발사 (최대 3발)
    if (m_iFireCount < 3 && (int)(m_fStateTimer * 5) > m_iFireCount)
    {
        CreateFireProjectile();
        m_iFireCount++;
    }
}

void CHotHead::SetupAnimationMapping()
{
}

void CHotHead::SpitFire()
{
    // 화염 뿜기 로직
    m_iFireCount = 0;

    // TODO: 화염 발사 사운드
    // TODO: 화염 발사 이펙트
}

void CHotHead::CreateFireProjectile()
{
    // TODO: 화염 투사체 오브젝트 생성
    // CFire* pFire = new CFire;
    // pFire->SetPos(GetPos() + Vec2(m_iDir * 20.f, -10.f));  // 입 위치에서 발사
    // pFire->SetDirection(Vec2(m_iDir, -0.2f));  // 약간 위쪽으로
    // pFire->SetSpeed(200.f);
    // 
    // CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
    // pCurScene->AddObject(pFire, GROUP_TYPE::PROJECTILE);
}
