#include "gamePCH.h"
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

    // 벽과 충돌했으면 방향 전환 (충돌 콜백에서 이미 방향이 바뀌었지만 상태도 변경)
    if (m_bWallCollision && !m_bPrevWallCollision)
    {
        ChangeState(MONSTER_STATE::TURN);
        return;
    }

    // 바닥과 충돌하지 않으면 방향 전환 (낭떠러지 감지)
    if (!m_bGroundCollision && m_bPrevGroundCollision)
    {
        ChangeState(MONSTER_STATE::TURN);
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