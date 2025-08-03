#pragma once
#include "CObject.h"

class CAnimator;
class CAnimation;
class CRigidBody;
class CPlayerStateMachine;
class CPlayerInhaleSystem;
class CPlayerMovement;

class CPlayer : public CObject
{
private:
    // === 컴포넌트들 ===
    CAnimator* m_pAnimator;                 // 애니메이터 컴포넌트
    CRigidBody* m_pRigidBody;               // 리지드바디 컴포넌트
    CPlayerStateMachine* m_pStateMachine;   // 상태 관리 시스템
    CPlayerInhaleSystem* m_pInhaleSystem;   // 빨아들이기 시스템
    CPlayerMovement* m_pMovement;           // 이동 시스템 (새로 추가)

public:
    CPlayer();
    ~CPlayer();

public:
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    virtual void OnCollisionEnter(CCollider* _pOther) override;
    virtual void OnCollision(CCollider* _pOther) override;
    virtual void OnCollisionExit(CCollider* _pOther) override;

    // === 상태 관련 인터페이스 ===
    PLAYER_STATE GetCurrentState() const;
    PLAYER_STATE GetPreviousState() const;
    void ChangeState(PLAYER_STATE _eState);

    // === 시스템 접근자들 ===
    CPlayerInhaleSystem* GetInhaleSystem() { return m_pInhaleSystem; }
    CPlayerMovement* GetMovement() { return m_pMovement; }  // 새로 추가

    // === 빨아들이기 관련 래퍼 함수들 (기존 인터페이스 유지) ===
    bool IsInhaling() const;
    bool HasMouthful() const;
    void SetMouthful(bool _bMouthful);
    float GetInhaleTime() const;
    const vector<CObject*>& GetInhaleTargets() const;
    OBJECT_TYPE GetMouthfulType() const;

    void StartInhale();
    void StopInhale();
    void SwallowTarget(CObject* _pTarget);
    void SpitOut();
    void ReleaseMouthful();

    // === 이동 관련 래퍼 함수들 (기존 인터페이스 유지) ===
    bool IsFacingRight() const;
    bool IsActuallyMoving() const;
    bool IsInputPressed() const;
    bool IsRunMode() const;
    void SetRunMode(bool _bRunMode);
    float GetCurrentSpeed() const;
    void SetFacingDirection(bool _bRight);
    bool IsDecelerating() const;

private:
    void RenderFlippedAnimation(HDC _dc, CAnimation* _pAnim, Vec2 _vRenderPos);

    // === 흡입 관련 업데이트 ===
    void UpdateInhale();

    // === 애니메이션 생성 함수 ===
    void CreateAnimation();
};