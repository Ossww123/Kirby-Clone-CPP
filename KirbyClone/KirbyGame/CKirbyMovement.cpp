#include "gamePCH.h"
#include "CKirbyMovement.h"
#include "CKirby.h"
#include "CRigidBody.h"
#include "CPlayerInputManager.h"
#include "CTimeMgr.h"

CKirbyMovement::CKirbyMovement(CKirby* owner)
    : m_owner(owner) {}

CRigidBody* CKirbyMovement::RB() const {
    return m_owner ? m_owner->GetRigidBody() : nullptr;
}

bool CKirbyMovement::IsGrounded() const {
    if (auto* rb = RB()) return rb->IsGround();
    return false;
}

float CKirbyMovement::GetVelocityX() const {
    if (auto* rb = RB()) return rb->GetVelocity().x;
    return 0.f;
}

void CKirbyMovement::Update(float dt) {
    (void)dt;
    // 지금은 상태 머신에서 직접 호출하므로, 기본 루프는 비워둬도 OK.
    // 필요하면 입력 기반의 공통 이동 처리(예: 공중 감속) 등을 여기에 추가.
}

void CKirbyMovement::ApplyTargetSpeed(float targetSpeed, float dt, bool air) {
    auto* rb = RB();
    if (!rb) return;

    const float cur = rb->GetVelocity().x;
    const float diff = targetSpeed - cur;

    float accel = air ? m_cfg.accelAir : m_cfg.accelGround;
    float add = accel * dt;

    float next = cur;
    if (diff > 0.f)       next = cur + min(add, diff);
    else if (diff < 0.f)  next = cur - min(add, -diff);

    rb->SetVelocityX(next);
}

void CKirbyMovement::MoveWalk(int dir) {
    if (dir == 0) { StopHorizontal(); return; }
    if (m_owner) m_owner->SetFacingRight(dir > 0);
    float dt = CTimeMgr::GetInst()->GetfDT();
    ApplyTargetSpeed(dir > 0 ? m_cfg.walkSpeed : -m_cfg.walkSpeed, dt, !IsGrounded());
}

void CKirbyMovement::MoveRun(int dir) {
    if (dir == 0) { StopHorizontal(); return; }
    if (m_owner) m_owner->SetFacingRight(dir > 0);
    float dt = CTimeMgr::GetInst()->GetfDT();
    ApplyTargetSpeed(dir > 0 ? m_cfg.runSpeed : -m_cfg.runSpeed, dt, !IsGrounded());
}

void CKirbyMovement::StopHorizontal() {
    auto* rb = RB();
    if (!rb) return;
    float dt = CTimeMgr::GetInst()->GetfDT();

    float cur = rb->GetVelocity().x;
    float sign = (cur > 0.f) ? 1.f : (cur < 0.f ? -1.f : 0.f);
    float mag = (std::max)(0.f, std::fabs(cur) - m_cfg.stopDrag * dt);
    rb->SetVelocityX(mag * sign);
}

void CKirbyMovement::Jump() {
    auto* rb = RB();
    if (!rb) return;
    rb->SetGround(false);
    rb->SetVelocityY(m_cfg.jumpVelocity);
}

void CKirbyMovement::SlideKickRecoil() {
    auto* rb = RB(); if (!rb) return;
    float dir = (m_owner && m_owner->IsFacingRight()) ? -1.f : 1.f;
    rb->SetVelocityX(dir * 260.f);
    rb->SetVelocityY(-120.f);
    rb->SetGround(false);
}
