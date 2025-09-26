#include "gamePCH.h"
#include "CKirbyHealthSystem.h"
#include "CKirby.h"
#include "CRigidBody.h"
#include "CAnimator.h"
#include "CEventMgr.h"
#include "CTimeMgr.h"

CKirbyHealthSystem::CKirbyHealthSystem(CKirby* owner)
    : m_owner(owner)
{
}

CKirbyHealthSystem::~CKirbyHealthSystem() = default;

void CKirbyHealthSystem::Update(float dt) {
    if (m_invincibleTime > 0.f) {
        m_invincibleTime = (std::max)(0.f, m_invincibleTime - dt);
        m_blinkTimer += dt;
        // 간단 점멸 연출이 필요하면 Animator에 훅 추가(없다면 생략)
        // if (auto* an = m_owner->GetAnimator()) { an->SetVisible( fmodf(m_blinkTimer, 0.1f) < 0.05f ); }
    }
    else {
        // if (auto* an = m_owner->GetAnimator()) an->SetVisible(true);
    }
}

void CKirbyHealthSystem::TakeDamage(int dmg, const Vec2& knockDir) {
    if (dmg <= 0 || IsInvincible() || !IsAlive()) return;

    m_hp = (std::max)(0, m_hp - dmg);
    m_invincibleTime = m_invincibleDuration;
    m_blinkTimer = 0.f;

    ApplyKnockback(knockDir);

    if (!IsAlive()) {
        OnDeath();
    }
    else {
        // 필요하면 “피해 받음” 이벤트를 브로드캐스트 (UI 등)
        // tEvent e{ EVENT_TYPE::PLAYER_DAMAGE_TAKEN, (DWORD_PTR)m_owner, 0 };
        // CEventMgr::GetInst()->AddEvent(e);
    }
}

void CKirbyHealthSystem::Heal(int amount) {
    if (amount <= 0 || !IsAlive()) return;
    m_hp = (std::min)(m_maxHp, m_hp + amount);
}

void CKirbyHealthSystem::ApplyKnockback(const Vec2& dir) {
    if (!m_owner) return;
    auto* rb = m_owner->GetRigidBody();
    if (!rb) return;

    Vec2 nd = dir;
    if (nd.x == 0.f && nd.y == 0.f) {
        // 기본 넉백: 바라보는 반대 방향 + 위로
        const bool right = m_owner->IsFacingRight();
        nd = Vec2(right ? -1.f : 1.f, -0.2f);
    }
    // 정규화 대충(0 길이면 위에서 이미 조정)
    const float len = sqrtf(nd.x * nd.x + nd.y * nd.y);
    if (len > 0.0001f) nd = nd * (1.f / len);

    rb->SetVelocityX(nd.x * m_knockPower);
    rb->SetVelocityY(nd.y * m_knockPower);
    rb->SetGround(false);
}

void CKirbyHealthSystem::OnDeath() {
    // “PLAYER_DEATH” 이벤트 발행 → 게임플로우/씬/페이드/사운드 등은 외부 구독자가 처리
    tEvent e{};
    e.eType = EVENT_TYPE::PLAYER_DEATH;
    e.wParam = (DWORD_PTR)m_owner;
    e.lParam = 0;
    CEventMgr::GetInst()->AddEvent(e);
}
