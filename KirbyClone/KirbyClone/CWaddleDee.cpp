#include "pch.h"
#include "CWaddleDee.h"
#include "CRigidBody.h"

CWaddleDee::CWaddleDee ( )
{
    // 오브젝트 타입 설정
    SetType ( OBJECT_TYPE::MONSTER_WADDLE_DEE );

    // 이동 속도 값 설정
    m_fSpeed = 80.f;

    // 애니메이션 로드
    LoadAnimationsFromFile ( L"waddle_dee_animations.json" );

    // 초기 상태 설정
    ChangeState ( MONSTER_STATE::IDLE );
}

CWaddleDee::~CWaddleDee ( )
{
    // 부모 클래스에서 처리
}

void CWaddleDee::Move ( )
{
    // TURN 상태이거나 특정 상태에서는 Move 로직 실행하지 않음
    MONSTER_STATE eCurrentState = GetCurrentState ( );
    if ( eCurrentState == MONSTER_STATE::TURN ||
        eCurrentState == MONSTER_STATE::DAMAGE ||
        eCurrentState == MONSTER_STATE::BEING_INHALED )
    {
        return;
    }

    // 벽 체크와 바닥 체크 결과 확인
    bool wallAhead = CheckWallAhead ( );
    bool groundAhead = CheckGroundAhead ( );

    // 앞방에 벽이 있거나 바닥이 없으면 방향 전환
    if ( wallAhead || !groundAhead )
    {
        ChangeState ( MONSTER_STATE::TURN );
        return;
    }

    // 계속 걷기
    MoveHorizontal ( m_fSpeed );
}

void CWaddleDee::SetupAnimationMapping ( )
{
    // 와들디 상태별 및 애니메이션 매핑 설정
    m_mapStateToAnimation[ MONSTER_STATE::IDLE ] = L"IDLE";
    m_mapStateToAnimation[ MONSTER_STATE::WALK ] = L"WALK";
    m_mapStateToAnimation[ MONSTER_STATE::TURN ] = L"WALK";
    m_mapStateToAnimation[ MONSTER_STATE::DAMAGE ] = L"DAMAGE";
}

void CWaddleDee::ChangeDirection ( )
{
    // 단순한 방향 전환
    TurnAround ( );
}