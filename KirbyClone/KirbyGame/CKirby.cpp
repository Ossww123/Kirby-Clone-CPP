#include "gamePCH.h"
#include "CKirby.h"
#include "CKirbyMovement.h"
#include "CKirbyHealthSystem.h"

#include "CAnimator.h"
#include "CRigidBody.h"
#include "CCollider.h"
#include "CPlayerInputManager.h"
#include "CTimeMgr.h"

#define PLAY(animName, loop) do { if(ctx.anim) (ctx.anim)->Play(L##animName, loop); } while(0)

// ===== States =====
class StGrounded : public KirbyState {
public:
    StGrounded() : KirbyState(KIRBY_STATE::GROUNDED) {}
    void OnEnter(KirbyStateCtx& ctx) override { if (ctx.body) ctx.body->SetGround(true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.input && ctx.input->IsJumpTap()) { out = KIRBY_STATE::JUMP; return; }
        if (ctx.body && !ctx.body->IsGround()) { out = KIRBY_STATE::FALL; }
    }
};

class StAirborne : public KirbyState {
public:
    StAirborne() : KirbyState(KIRBY_STATE::AIRBORNE) {}
    void OnEnter(KirbyStateCtx& ctx) override { if (ctx.body) ctx.body->SetGround(false); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.body && ctx.body->IsGround()) { out = KIRBY_STATE::IDLE; }
    }
};

class StIdle : public KirbyState {
public:
    StIdle() : KirbyState(KIRBY_STATE::IDLE) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("IDLE", true); if (ctx.move) ctx.move->StopHorizontal(); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input) return;
        if (ctx.input->IsMovingLeft() || ctx.input->IsMovingRight()) { out = KIRBY_STATE::WALK; return; }
        if (ctx.input->IsMovingDown()) { out = KIRBY_STATE::CROUCH; return; }
    }
};

class StWalk : public KirbyState {
public:
    StWalk() : KirbyState(KIRBY_STATE::WALK) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("WALK", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input || !ctx.move) return;
        const int dir = ctx.input->GetHorizontalInput();
        if (dir == 0) { out = KIRBY_STATE::IDLE; return; }
        if (ctx.input->IsDoubleTapLeft() || ctx.input->IsDoubleTapRight()) { out = KIRBY_STATE::RUN; return; }
        ctx.move->MoveWalk(dir);
    }
};

class StRun : public KirbyState {
public:
    StRun() : KirbyState(KIRBY_STATE::RUN) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("RUN", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input || !ctx.move) return;
        const int dir = ctx.input->GetHorizontalInput();
        if (dir == 0) { out = KIRBY_STATE::IDLE; return; }
        ctx.move->MoveRun(dir);
    }
};

class StCrouch : public KirbyState {
public:
    StCrouch() : KirbyState(KIRBY_STATE::CROUCH) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("CROUCH", true); if (ctx.move) ctx.move->StopHorizontal(); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input) return;
        if (!ctx.input->IsMovingDown()) { out = KIRBY_STATE::IDLE; return; }
    }
};

class StJump : public KirbyState {
public:
    StJump() : KirbyState(KIRBY_STATE::JUMP) {}
    void OnEnter(KirbyStateCtx& ctx) override {
        PLAY("JUMP", false);
        if (ctx.move) ctx.move->Jump();
    }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.body && ctx.body->GetVelocity().y > 0.f) { out = KIRBY_STATE::FALL; }
    }
};

class StFall : public KirbyState {
public:
    StFall() : KirbyState(KIRBY_STATE::FALL) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("FALL", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.body && ctx.body->IsGround()) { out = KIRBY_STATE::IDLE; }
    }
};

// ===== CKirby =====
CKirby::CKirby() {
    SetType(OBJECT_TYPE::PLAYER);

    CreateAnimator();
    CreateRigidBody();
    CreateCollider();

    if (auto* rb = GetRigidBody()) {
        rb->SetMass(1.f);
        rb->SetMaxVelocity(640.f);
        rb->SetFriction(0.1f);
        rb->SetUseGravity(true);
    }
    if (auto* col = GetCollider()) {
        col->SetOffsetPos(Vec2(0.f, 0.f));
        col->SetScale(m_vNormalCollider);
    }

    m_input = std::make_unique<CPlayerInputManager>();
    m_move = std::make_unique<CKirbyMovement>(this);
    m_health = std::make_unique<CKirbyHealthSystem>(this);

    LoadDefaultAnimations();
    BuildHFSM();
    ChangeState(KIRBY_STATE::IDLE);

    SetPos(Vec2(640.f, 384.f));
    SetScale(Vec2(64.f, 64.f));
}

CKirby::~CKirby() {}

void CKirby::LoadDefaultAnimations() {
    if (auto* ani = GetAnimator()) ani->Play(L"IDLE", true);
}

void CKirby::LoadAbilityAnimations(int) {}

KirbyState* CKirby::FindNode(KirbyState* node, KIRBY_STATE id) {
    if (!node) return nullptr;
    if (node->id == id) return node;
    for (auto& ch : node->children) {
        if (auto* f = FindNode(ch.get(), id)) return f;
    }
    return nullptr;
}

std::vector<KirbyState*> CKirby::BuildPathToRoot(KirbyState* n) {
    std::vector<KirbyState*> v; while (n) { v.push_back(n); n = n->parent; }
    std::reverse(v.begin(), v.end()); return v;
}

KirbyState* CKirby::LCA(KirbyState* a, KirbyState* b) {
    auto pa = BuildPathToRoot(a), pb = BuildPathToRoot(b);
    KirbyState* last = nullptr; size_t i = 0;
    while (i < pa.size() && i < pb.size() && pa[i] == pb[i]) { last = pa[i]; ++i; }
    return last;
}

void CKirby::ResetForRespawn(bool briefInvincible)
{
    if (auto* hs = GetHealth()) {
        const int maxhp = hs->GetMaxHP();
        hs->SetHP(maxhp);
        hs->ClearInvincibility();
        if (briefInvincible) hs->StartInvincible(0.5f); // 짧은 시작 무적
    }

    // TODO(능력 시스템 붙을 때): 카피 능력 초기화가 필요하면 여기서 처리
    // ex) SetCopyAbility(COPY_ABILITY::NONE);

    // 속도/접지 상태 초기화
    if (auto* rb = GetRigidBody()) {
        rb->SetVelocity(Vec2(0.f, 0.f));
        rb->SetGround(false);
    }
}

void CKirby::BuildHFSM() {
    m_root = std::make_unique<KirbyState>(KIRBY_STATE::ROOT);

    m_nodeGROUNDED = m_root->AddChild(std::make_unique<StGrounded>());
    m_nodeAIRBORNE = m_root->AddChild(std::make_unique<StAirborne>());

    m_nodeIDLE = m_nodeGROUNDED->AddChild(std::make_unique<StIdle>());
    m_nodeWALK = m_nodeGROUNDED->AddChild(std::make_unique<StWalk>());
    m_nodeRUN = m_nodeGROUNDED->AddChild(std::make_unique<StRun>());
    m_nodeCROUCH = m_nodeGROUNDED->AddChild(std::make_unique<StCrouch>());

    m_nodeJUMP = m_nodeAIRBORNE->AddChild(std::make_unique<StJump>());
    m_nodeFALL = m_nodeAIRBORNE->AddChild(std::make_unique<StFall>());

    m_nodeROOT = m_root.get();
}

void CKirby::ChangeState(KIRBY_STATE target) {
    KirbyState* to = FindNode(m_root.get(), target);
    if (!to) return;

    KirbyState* from = m_curNode;
    if (!from) {
        auto path = BuildPathToRoot(to);
        for (auto* n : path) n->OnEnter(m_ctx);
        m_curNode = to; m_curLeaf = target; return;
    }

    KirbyState* lca = LCA(from, to);
    for (KirbyState* n = from; n && n != lca; n = n->parent) n->OnExit(m_ctx);

    auto pathTo = BuildPathToRoot(to);
    auto pathFrom = BuildPathToRoot(from);
    size_t skip = 0; while (skip < pathTo.size() && skip < pathFrom.size() && pathTo[skip] == pathFrom[skip]) ++skip;
    for (size_t i = skip; i < pathTo.size(); ++i) pathTo[i]->OnEnter(m_ctx);

    m_curNode = to; m_curLeaf = target;
}

void CKirby::Update() {
    if (m_input) m_input->Update();

    m_ctx.self = this;
    m_ctx.input = m_input.get();
    m_ctx.body = GetRigidBody();
    m_ctx.anim = GetAnimator();
    m_ctx.move = m_move.get();

    if (m_curNode) {
        const float dt = CTimeMgr::GetInst()->GetfDT();
        KIRBY_STATE req = KIRBY_STATE::END;
        m_curNode->Update(m_ctx, dt, req);
        if (req != KIRBY_STATE::END && req != m_curLeaf) ChangeState(req);
    }

    // 서브시스템 업데이트
    if (m_move)   m_move->Update(CTimeMgr::GetInst()->GetfDT());
    if (m_health) m_health->Update(CTimeMgr::GetInst()->GetfDT());

    UpdateColliderSize();

    if (auto* rb = GetRigidBody()) rb->Update();
    if (auto* an = GetAnimator())  an->Update();
}

void CKirby::Render(HDC dc) {
    if (auto* an = GetAnimator()) {
        an->SetFlipX(!IsFacingRight());
        an->Render(dc);
    }
    else {
        CObject::Render(dc);
    }
    // 디버그 콜라이더 그리기 원하면: if (GetCollider()) GetCollider()->RenderScaled(dc, 1.0f);
}

void CKirby::OnCollisionEnter(CCollider* other) { (void)other; }
void CKirby::OnCollision(CCollider* other) { (void)other; }
void CKirby::OnCollisionExit(CCollider* other) { (void)other; }

void CKirby::UpdateColliderSize() {
    if (!GetCollider()) return;
    const bool isCrouch = (m_curLeaf == KIRBY_STATE::CROUCH);
    const Vec2 target = isCrouch ? m_vCrouchCollider : m_vNormalCollider;

    const Vec2 cur = GetCollider()->GetScale();
    if (cur != target) {
        GetCollider()->SetScale(target);
        AdjustPositionForColliderResize(cur, target);
    }
}

void CKirby::AdjustPositionForColliderResize(const Vec2& oldS, const Vec2& newS) {
    auto* rb = GetRigidBody();
    if (!rb || !rb->IsGround()) return;

    const float diffY = oldS.y - newS.y;
    if (fabsf(diffY) > 0.1f) {
        Vec2 p = GetPos();
        p.y += diffY * 0.5f;
        SetPos(p);
    }
}
