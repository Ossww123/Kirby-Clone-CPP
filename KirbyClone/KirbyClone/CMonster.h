#pragma once
#include "CObject.h"
#include "enum.h"

class CAnimator;
class CRigidBody;
class CTexture;

class CMonster : public CObject
{
private:
    CAnimator* m_pAnimator;        // 애니메이터 참조
    CRigidBody* m_pRigidBody;       // 리지드바디 참조

    MONSTER_STATE   m_eCurState;        // 현재 상태
    MONSTER_STATE   m_ePrevState;       // 이전 상태
    OBJECT_TYPE     m_eMonsterType;     // 몬스터 타입

    float           m_fSpeed;           // 이동 속도
    int             m_iDir;             // 이동 방향 (-1: 왼쪽, 1: 오른쪽)
    float           m_fStateTimer;      // 상태 타이머
    float           m_fIdleTime;        // 대기 시간

    bool            m_bHitWall;         // 벽 충돌 여부
    float           m_fGroundCheckDist; // 바닥 체크 거리
    float           m_fWallCheckDist;   // 벽 체크 거리

public:
    CMonster();
    virtual ~CMonster();

public:
    virtual void Update() override;

    // 몬스터 설정
    void SetMonsterType(OBJECT_TYPE _eType);
    OBJECT_TYPE GetMonsterType() const { return m_eMonsterType; }

    // 상태 관리
    void ChangeState(MONSTER_STATE _eState);
    MONSTER_STATE GetCurrentState() const { return m_eCurState; }
    MONSTER_STATE GetPreviousState() const { return m_ePrevState; }

    // 액션
    void TakeDamage();
    void TurnAround();

    // 충돌 체크
    bool CheckWallAhead();
    bool CheckGroundAhead();

private:
    // 애니메이션 생성
    void CreateAnimation();
    void CreateWaddleDeeAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);
    void CreateWaddleDooAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);
    void CreateBrontoBurtAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);
    void CreateGordosAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);
    void CreateHotHeadAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);
    void CreateSparkyAnimations(CTexture* pTex, Vec2 frameSize, Vec2 frameOffset, float frameDuration);

    // 상태 업데이트
    void UpdateState();
    void UpdateMove();

    // 개별 상태 업데이트 함수들
    void UpdateIdle();
    void UpdateWalk();
    void UpdateFly();
    void UpdateTurn();
    void UpdateDamage();
    void UpdateAttackReady();
    void UpdateAttack();
};