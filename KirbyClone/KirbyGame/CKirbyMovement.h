#pragma once
#include <cstdint>

class CKirby;
class CRigidBody;
class CPlayerInputManager;

struct KirbyMoveConfig {
    float walkSpeed = 150.f;
    float runSpeed = 300.f;
    float accelGround = 1800.f;
    float accelAir = 1200.f;
    float stopDrag = 2200.f;   // 정지 가속도(마찰 보조)
    float jumpVelocity = -640.f;
};

class CKirbyMovement {
public:
    explicit CKirbyMovement(CKirby* owner);

    void SetConfig(const KirbyMoveConfig& cfg) { m_cfg = cfg; }
    const KirbyMoveConfig& GetConfig() const { return m_cfg; }

    // 매 프레임 호출(선택): 입력 기반 기본 처리 넣고 싶을 때 사용
    void Update(float dt);

    // 상태(STATE)에서 호출하는 동작들
    void MoveWalk(int dir);   // dir: -1,0,1
    void MoveRun(int dir);
    void StopHorizontal();
    void Jump();
    void SlideKickRecoil();

    // 쿼리
    bool IsGrounded() const;
    float GetVelocityX() const;

private:
    CRigidBody* RB() const; // owner의 RB 접근
    void ApplyTargetSpeed(float targetSpeed, float dt, bool air);

private:
    CKirby* m_owner{ nullptr };
    KirbyMoveConfig  m_cfg{};
};
