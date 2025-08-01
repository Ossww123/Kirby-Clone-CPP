#pragma once
#include "CObject.h"

class CAnimator;
class CRigidBody;

class CPlayer : public CObject
{
private:
    CAnimator*      m_pAnimator;        // 애니메이터 컴포넌트
    CRigidBody*     m_pRigidBody;       // 리지드바디 컴포넌트
    PLAYER_STATE    m_eCurState;        // 현재 상태
    PLAYER_STATE    m_ePrevState;       // 이전 상태

    float           m_fSpeed;           // 이동 속도
    float           m_fRunSpeed;        // 달리기 속도
    float           m_fJumpPower;       // 점프력

    // 빨아들이기 관련
    bool            m_bInhaling;        // 빨아들이기 중인지
    float           m_fInhaleTime;      // 빨아들이기 지속 시간
    bool            m_bHasMouthful;     // 입에 뭔가 머금고 있는지

    // 빨아들이기 범위와 효과
    float           m_fInhaleRange;     // 빨아들이기 범위
    Vec2            m_vInhaleDir;       // 빨아들이기 방향
    vector<CObject*> m_vecInhaleTargets; // 빨아들이기 대상들

    // 머금은 적의 정보
    CObject* m_pMouthfulTarget;  // 머금고 있는 적
    OBJECT_TYPE     m_eMouthfulType;    // 머금은 적의 타입

    // 효과음 및 이펙트
    bool            m_bPlayingInhaleEffect; // 빨아들이기 이펙트 재생 중

    // 입력 관련
    bool            m_bRunMode;         // 달리기 모드인지

public:
    virtual void Update();
    virtual void Render(HDC _dc);

    virtual void OnCollisionEnter(CCollider* _pOther);
    virtual void OnCollision(CCollider* _pOther);
    virtual void OnCollisionExit(CCollider* _pOther);

    bool IsInhaling() { return m_bInhaling; }
    bool HasMouthful() { return m_bHasMouthful; }
    void SetMouthful(bool _bMouthful) { m_bHasMouthful = _bMouthful; }

private:
    void CreateAnimation();             // 애니메이션 생성
    void UpdateState();                 // 상태 업데이트
    void UpdateMove();                  // 이동 처리
    void ChangeState(PLAYER_STATE _eState); // 상태 변경

public:
    // 빨아들이기 관련 함수들
    void StartInhale();
    void UpdateInhale();
    void StopInhale();
    void SwallowTarget(CObject* _pTarget);
    void SpitOut();
    void RenderInhaleEffect(HDC _dc);

    // 머금은 상태 관리
    void ReleaseMouthful();
    OBJECT_TYPE GetMouthfulType() { return m_eMouthfulType; }

public:
    CPlayer();
    ~CPlayer();
};