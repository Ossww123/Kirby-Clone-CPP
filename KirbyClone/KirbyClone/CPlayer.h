#pragma once
#include "CObject.h"

class CAnimator;

class CPlayer : public CObject
{
private:
    CAnimator*      m_pAnimator;        // 애니메이터 컴포넌트
    PLAYER_STATE    m_eCurState;        // 현재 상태
    PLAYER_STATE    m_ePrevState;       // 이전 상태

    Vec2            m_vVelocity;        // 속도 벡터
    float           m_fSpeed;           // 이동 속도
    float           m_fJumpPower;       // 점프력
    bool            m_bGround;          // 바닥에 있는지 여부
    float           m_fGravity;         // 중력

public:
    virtual void Update();
    virtual void Render(HDC _dc);

    virtual void OnCollisionEnter(CCollider* _pOther);
    virtual void OnCollision(CCollider* _pOther);
    virtual void OnCollisionExit(CCollider* _pOther);

private:
    void CreateAnimation();             // 애니메이션 생성
    void UpdateState();                 // 상태 업데이트
    void UpdateMove();                  // 이동 처리
    void ChangeState(PLAYER_STATE _eState); // 상태 변경

public:
    CPlayer();
    ~CPlayer();
};