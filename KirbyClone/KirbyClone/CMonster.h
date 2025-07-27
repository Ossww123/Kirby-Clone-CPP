#pragma once
#include "CObject.h"

class CAnimator;
class CRigidBody;

class CMonster : public CObject
{
private:
    CAnimator* m_pAnimator;        // 애니메이터 컴포넌트
    CRigidBody* m_pRigidBody;       // 리지드바디 컴포넌트

    MONSTER_STATE   m_eCurState;        // 현재 상태
    MONSTER_STATE   m_ePrevState;       // 이전 상태

    float           m_fSpeed;           // 이동 속도
    int             m_iDir;             // 이동 방향 (1: 오른쪽, -1: 왼쪽)

    // AI 관련
    float           m_fStateTimer;      // 상태 지속 시간
    float           m_fIdleTime;        // 정지 시간
    bool            m_bHitWall;         // 벽에 부딪혔는지 여부

    // 벽 감지용
    float           m_fGroundCheckDist; // 바닥 체크 거리
    float           m_fWallCheckDist;   // 벽 체크 거리

public:
    virtual void Update();
    virtual void Render(HDC _dc);

    // 충돌 처리
    virtual void OnCollisionEnter(CCollider* _pOther);
    virtual void OnCollision(CCollider* _pOther);
    virtual void OnCollisionExit(CCollider* _pOther);

private:
    void CreateAnimation();             // 애니메이션 생성
    void UpdateState();                 // 상태 업데이트 (FSM)
    void UpdateMove();                  // 이동 처리
    void ChangeState(MONSTER_STATE _eState); // 상태 변경

    // AI 관련 함수들
    void UpdateIdle();                  // 정지 상태 처리
    void UpdateWalk();                  // 걷기 상태 처리
    void UpdateTurn();                  // 방향 전환 처리

    // 벽/바닥 감지
    bool CheckWallAhead();              // 전방 벽 감지
    bool CheckGroundAhead();            // 전방 바닥 감지 (절벽 방지)
    void TurnAround();                  // 방향 전환

public:
    CMonster();
    ~CMonster();
};