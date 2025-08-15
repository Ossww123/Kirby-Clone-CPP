#include "pch.h"
#include "CWaddleDee.h"

CWaddleDee::CWaddleDee()
{
    // 오브젝트 타입 설정
    SetType(OBJECT_TYPE::MONSTER_WADDLE_DEE);

    // 웨이들 디 전용 설정
    m_fSpeed = 80.f;

    // 애니메이션 로드
    LoadAnimationsFromFile(L"waddle_dee_animations.json");

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

void CWaddleDee::SetupAnimationMapping()
{
    // 웨이들 디의 상태 → 애니메이션 매핑 설정
    m_mapStateToAnimation[MONSTER_STATE::IDLE] = L"IDLE";
    m_mapStateToAnimation[MONSTER_STATE::WALK] = L"WALK";
    m_mapStateToAnimation[MONSTER_STATE::TURN] = L"WALK";  // 방향전환 시에도 걷기 애니메이션
    m_mapStateToAnimation[MONSTER_STATE::DAMAGE] = L"DAMAGE";
}

void CWaddleDee::ChangeDirection()
{
    // 단순한 방향 전환
    TurnAround();
}