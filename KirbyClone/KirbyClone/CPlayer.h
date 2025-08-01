#pragma once
#include "CObject.h"

class CAnimator;
class CAnimation;
class CRigidBody;

class CPlayer : public CObject
{
private:
    CAnimator* m_pAnimator;        // 애니메이터 컴포넌트
    CRigidBody* m_pRigidBody;       // 리지드바디 컴포넌트
    PLAYER_STATE    m_eCurState;        // 현재 상태
    PLAYER_STATE    m_ePrevState;       // 이전 상태

    float           m_fSpeed;           // 이동 속도
    float           m_fRunSpeed;        // 달리기 속도
    float           m_fJumpPower;       // 점프력

    // 흡입하기 관련
    bool            m_bInhaling;        // 흡입하기 중인지
    float           m_fInhaleTime;      // 흡입하기 지속 시간
    bool            m_bHasMouthful;     // 입에 뭔가 물고 있는지

    // 흡입하기 범위와 효과
    float           m_fInhaleRange;     // 흡입하기 범위
    Vec2            m_vInhaleDir;       // 흡입하기 방향
    vector<CObject*> m_vecInhaleTargets; // 흡입하기 대상들

    // 물고 있는 적의 정보
    CObject* m_pMouthfulTarget;  // 물고 있는 적
    OBJECT_TYPE     m_eMouthfulType;    // 물고 있는 적의 타입

    // 효과음 및 이펙트
    bool            m_bPlayingInhaleEffect; // 흡입하기 이펙트 재생 중

    // 입력 관련
    bool            m_bRunMode;         // 달리기 모드인지

    // === 방향 시스템 관련 변수들 ===
    bool            m_bFacingRight;     // 오른쪽을 보고 있는지 (true: 오른쪽, false: 왼쪽)
    int             m_iLastMoveDir;     // 마지막 이동 방향 (1: 오른쪽, -1: 왼쪽, 0: 정지)
    bool            m_bDirectionChanged; // 방향이 바뀌었는지 체크

    // === 부드러운 움직임 관련 변수들 ===
    float           m_fDeceleration;        // 감속도 (키를 뗐을 때)
    float           m_fMinMovingSpeed;      // 최소 이동 속도 (이 이하면 정지로 간주)
    bool            m_bIsDecelerating;      // 현재 감속 중인지
    float           m_fDecelTimer;          // 감속 타이머

    // 입력 상태 추적
    bool            m_bWasMovingLastFrame;  // 이전 프레임에 이동 중이었는지
    bool            m_bInputPressed;        // 현재 입력이 눌려있는지

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

    // === 방향 시스템 관련 함수들 ===
    void UpdateDirection();             // 방향 업데이트 (매 프레임 호출)
    void SetFacingDirection(bool _bRight); // 방향 설정 및 흡입 방향 동기화
    void UpdateInhaleDirection();       // 흡입 방향을 현재 바라보는 방향으로 업데이트
    void RenderFlippedAnimation(HDC _dc, CAnimation* _pAnim, Vec2 _vRenderPos);

    void UpdateMovementState();             // 움직임 상태 업데이트
    void ApplyDeceleration();               // 감속 적용
    bool IsActuallyMoving();                // 실제로 움직이고 있는지 확인


public:
    // 흡입하기 관련 함수들
    void StartInhale();
    void UpdateInhale();
    void StopInhale();
    void SwallowTarget(CObject* _pTarget);
    void SpitOut();
    void RenderInhaleEffect(HDC _dc);

    // 물고 있는 상태 관리
    void ReleaseMouthful();
    OBJECT_TYPE GetMouthfulType() { return m_eMouthfulType; }

    // 방향 관련 함수들
    bool IsFacingRight() { return m_bFacingRight; }
    void SetFacingRight(bool _bRight) { SetFacingDirection(_bRight); } // 내부적으로 SetFacingDirection 호출
    int GetLastMoveDir() { return m_iLastMoveDir; }

    // 방향이 바뀌었는지 확인 (디버깅용)
    bool HasDirectionChanged() { return m_bDirectionChanged; }

public:
    CPlayer();
    ~CPlayer();
};