#include "pch.h"
#include "CWaddleDee.h"

CWaddleDee::CWaddleDee()
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WADDLE_DEE);

    // 웨이들 디 전용 설정
    m_fSpeed = 80.f;

    // 애니메이션 생성
    CreateAnimations();

    // 초기 상태 설정
    ChangeState(MONSTER_STATE::IDLE);
}

CWaddleDee::~CWaddleDee()
{
    // 상위 클래스에서 정리
}

void CWaddleDee::Move()
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

void CWaddleDee::CreateAnimations()
{
    // 첫 번째 행: 웨이들 디 - 시작 위치 (8, 8)
    Vec2 startPos = Vec2(8.f, 8.f);

    // WALK 애니메이션 (1~8열)
    CreateBasicAnimation(L"WALK", startPos, 8, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, true);

    // DAMAGE 애니메이션 (9~12열) - 빨아들임 중에도 사용
    CreateBasicAnimation(L"DAMAGE", Vec2(startPos.x + 32.f * 8, startPos.y), 4,
        Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.15f, false);

    // IDLE 애니메이션
    CreateBasicAnimation(L"IDLE", startPos, 1, Vec2(32.f, 32.f), Vec2(32.f, 32.f), 0.5f, true);
}

void CWaddleDee::ChangeDirection()
{
    // 단순한 방향 전환
    TurnAround();
}