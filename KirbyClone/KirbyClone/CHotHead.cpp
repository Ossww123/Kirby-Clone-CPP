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
    CreateAnimations();

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

void CHotHead::CreateAnimations()
{
    // 다섯 번째 행: 핫 헤드 - 시작 위치 (8, 136)
    Vec2 startPos = Vec2(8.f, 8.f + 128.f);

    // WALK 애니메이션 (1~8열)
    CreateBasicAnimation(L"WALK", startPos, 8, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // DAMAGE 애니메이션 (9~12열)
    CreateBasicAnimation(L"DAMAGE", Vec2(startPos.x + 32.f * 8, startPos.y), 4,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // ATTACK_READY 애니메이션 (13열)
    CreateBasicAnimation(L"ATTACK_READY", Vec2(startPos.x + 32.f * 12, startPos.y), 1,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // ATTACK 애니메이션 (14~15열)
    CreateBasicAnimation(L"ATTACK", Vec2(startPos.x + 32.f * 13, startPos.y), 2,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // IDLE 애니메이션
    CreateBasicAnimation(L"IDLE", startPos, 1, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.5f, true);
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
