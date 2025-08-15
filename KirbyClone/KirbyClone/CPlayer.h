#pragma once
#include "CObject.h"

// 전방 선언
class CPlayerStateMachine;
class CPlayerInhaleSystem;
class CPlayerMovement;
class CPlayerHealthSystem;
class CPlayerCollisionSystem;

class CPlayer : public CObject
{
public:
    // === 생성자 & 소멸자 ===
    CPlayer();
    ~CPlayer();

    // === 핵심 생명주기 함수 ===
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // === 충돌 처리 ===
    virtual void OnCollisionEnter(CCollider* _pOther) override;
    virtual void OnCollision(CCollider* _pOther) override;
    virtual void OnCollisionExit(CCollider* _pOther) override;

    // === 상태 관리 인터페이스 ===
    PLAYER_STATE GetCurrentState() const;
    PLAYER_STATE GetPreviousState() const;
    void ChangeState(PLAYER_STATE _eState);
    void ChangeStateInternal(PLAYER_STATE _eState);

    // === 필수 래퍼 함수들 ===
    // 흡입 관련 필수 기능
    bool IsInhaling() const;
    bool HasMouthful() const;
    void StartInhale();
    void StopInhale();
    void SpitOut();

    // 이동 관련 필수 기능
    bool IsFacingRight() const;

    // 체력 관련 필수 기능
    void TakeDamage(int _iDamage = 1, Vec2 _vKnockbackDir = Vec2(0.f, 0.f));
    bool IsGameOver() const;
    bool ShouldRenderBlink() const;

    // === 시스템 접근자들 ===
    CPlayerInhaleSystem* GetInhaleSystem() const { return m_pInhaleSystem; }
    CPlayerMovement* GetMovement() const { return m_pMovement; }
    CPlayerHealthSystem* GetHealthSystem() const { return m_pHealthSystem; }
    CPlayerStateMachine* GetStateMachine() const { return m_pStateMachine; }

private:
    // === 렌더링 헬퍼 함수 ===
    void RenderInvincible(HDC _dc);

    // === 업데이트 헬퍼 함수 ===
    void UpdateInhale();

    // === 크라우치 관련 헬퍼 함수들 ===
    void UpdateColliderSize(); 
    void AdjustPositionForColliderResize(const Vec2& _vOldScale, const Vec2& _vNewScale);
    void InitiateSlidePhysics();

    // === 애니메이션 생성 함수 ===
    void CreateAnimation();

    // === 컴포넌트들 ===
    CPlayerStateMachine* m_pStateMachine;   // 상태 관리 시스템
    CPlayerInhaleSystem* m_pInhaleSystem;   // 흡입들이기 시스템
    CPlayerMovement* m_pMovement;           // 이동 시스템
    CPlayerHealthSystem* m_pHealthSystem;   // 체력 시스템
    CPlayerCollisionSystem* m_pCollisionSystem; // 충돌 처리 시스템

    // === 크라우치 관련 멤버 변수들 (새로 추가) ===
    Vec2 m_vNormalColliderScale;            // 일반 상태 충돌체 크기
    Vec2 m_vCrouchColliderScale;            // 크라우치 상태 충돌체 크기
};