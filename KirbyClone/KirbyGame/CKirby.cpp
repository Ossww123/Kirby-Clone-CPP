#include "gamePCH.h"
#include "CKirby.h"

// 엔진/프로젝트 헤더
#include "CAnimator.h"
#include "CRigidBody.h"
#include "CCollider.h"
#include "CPlayerInputManager.h"
#include "CTimeMgr.h"

// ===========================
// 간단한 애니 헬퍼
// ===========================
#define PLAY(animName, loop) do { if(ctx.anim) (ctx.anim)->Play(L##animName, loop); } while(0)

// ===========================
// 내부 상태 구현(기본 이동만)
// ===========================

class StGrounded : public KirbyState {
public:
    StGrounded() : KirbyState(KIRBY_STATE::GROUNDED) {}
    void OnEnter(KirbyStateCtx& ctx) override {
        if (ctx.body) ctx.body->SetGround(true);
    }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        // 점프 입력 공통 처리
        if (ctx.input && ctx.input->IsJumpTap()) {
            out = KIRBY_STATE::JUMP;
            return;
        }
        // 지면이 아니게 되면 낙하
        if (ctx.body && !ctx.body->IsGround()) {
            out = KIRBY_STATE::FALL;
        }
    }
};

class StAirborne : public KirbyState {
public:
    StAirborne() : KirbyState(KIRBY_STATE::AIRBORNE) {}
    void OnEnter(KirbyStateCtx& ctx) override {
        if (ctx.body) ctx.body->SetGround(false);
    }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        // 착지 시 지상 루트로 복귀
        if (ctx.body && ctx.body->IsGround()) {
            out = KIRBY_STATE::IDLE;
        }
    }
};

class StIdle : public KirbyState {
public:
    StIdle() : KirbyState(KIRBY_STATE::IDLE) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("IDLE", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input) return;
        if (ctx.input->IsMovingLeft() || ctx.input->IsMovingRight()) {
            out = KIRBY_STATE::WALK; return;
        }
        if (ctx.input->IsMovingDown()) {
            out = KIRBY_STATE::CROUCH; return;
        }
    }
};

class StWalk : public KirbyState {
public:
    StWalk() : KirbyState(KIRBY_STATE::WALK) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("WALK", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input || !ctx.body) return;

        const int dir = ctx.input->GetHorizontalInput(); // -1,0,1
        if (dir == 0) { out = KIRBY_STATE::IDLE; return; }

        // 런 전환(더블탭)
        if (ctx.input->IsDoubleTapLeft() || ctx.input->IsDoubleTapRight()) {
            out = KIRBY_STATE::RUN; return;
        }

        ctx.body->SetVelocityX(dir > 0 ? 150.f : -150.f);
        if (ctx.self) ctx.self->SetFacingRight(dir > 0);
    }
};

class StRun : public KirbyState {
public:
    StRun() : KirbyState(KIRBY_STATE::RUN) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("RUN", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input || !ctx.body) return;

        const int dir = ctx.input->GetHorizontalInput();
        if (dir == 0) { out = KIRBY_STATE::IDLE; return; }

        ctx.body->SetVelocityX(dir > 0 ? 300.f : -300.f);
        if (ctx.self) ctx.self->SetFacingRight(dir > 0);
    }
};

class StCrouch : public KirbyState {
public:
    StCrouch() : KirbyState(KIRBY_STATE::CROUCH) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("CROUCH", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (!ctx.input) return;
        if (!ctx.input->IsMovingDown()) {
            out = KIRBY_STATE::IDLE; return;
        }
    }
};

class StJump : public KirbyState {
public:
    StJump() : KirbyState(KIRBY_STATE::JUMP) {}
    void OnEnter(KirbyStateCtx& ctx) override {
        PLAY("JUMP", false);
        if (ctx.body) {
            ctx.body->SetGround(false);
            ctx.body->SetVelocityY(-640.f); // 초기 점프력
        }
    }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.body && ctx.body->GetVelocity().y > 0.f) {
            out = KIRBY_STATE::FALL;
        }
    }
};

class StFall : public KirbyState {
public:
    StFall() : KirbyState(KIRBY_STATE::FALL) {}
    void OnEnter(KirbyStateCtx& ctx) override { PLAY("FALL", true); }
    void Update(KirbyStateCtx& ctx, float, KIRBY_STATE& out) override {
        if (ctx.body && ctx.body->IsGround()) {
            out = KIRBY_STATE::IDLE;
        }
    }
};

// ===========================
// CKirby 구현
// ===========================

CKirby::CKirby() {
    SetType(OBJECT_TYPE::PLAYER);

    // 컴포넌트
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

    LoadDefaultAnimations();
    BuildHFSM();
    ChangeState(KIRBY_STATE::IDLE);

    // 초기 트랜스폼
    SetPos(Vec2(640.f, 384.f));
    SetScale(Vec2(64.f, 64.f));
}

CKirby::~CKirby() {}

void CKirby::LoadDefaultAnimations() {
    if (auto* ani = GetAnimator()) {
        ani->Play(L"IDLE", true);
    }
}

void CKirby::LoadAbilityAnimations(int) {
    // TODO: 카피 능력/폼에 따른 애니 클립 바인딩
}

KirbyState* CKirby::FindNode(KirbyState* node, KIRBY_STATE id) {
    if (!node) return nullptr;
    if (node->id == id) return node;
    for (auto& ch : node->children) {
        if (auto* f = FindNode(ch.get(), id)) return f;
    }
    return nullptr;
}

std::vector<KirbyState*> CKirby::BuildPathToRoot(KirbyState* n) {
    std::vector<KirbyState*> path;
    while (n) { path.push_back(n); n = n->parent; }
    std::reverse(path.begin(), path.end());
    return path;
}

KirbyState* CKirby::LCA(KirbyState* a, KirbyState* b) {
    auto pa = BuildPathToRoot(a);
    auto pb = BuildPathToRoot(b);
    KirbyState* last = nullptr;
    size_t i = 0;
    while (i < pa.size() && i < pb.size() && pa[i] == pb[i]) { last = pa[i]; ++i; }
    return last;
}

void CKirby::BuildHFSM() {
    m_root = std::make_unique<KirbyState>(KIRBY_STATE::ROOT);

    // 슈퍼 상태
    m_nodeGROUNDED = m_root->AddChild(std::make_unique<StGrounded>());
    m_nodeAIRBORNE = m_root->AddChild(std::make_unique<StAirborne>());

    // 지상 리프
    m_nodeIDLE = m_nodeGROUNDED->AddChild(std::make_unique<StIdle>());
    m_nodeWALK = m_nodeGROUNDED->AddChild(std::make_unique<StWalk>());
    m_nodeRUN = m_nodeGROUNDED->AddChild(std::make_unique<StRun>());
    m_nodeCROUCH = m_nodeGROUNDED->AddChild(std::make_unique<StCrouch>());

    // 공중 리프
    m_nodeJUMP = m_nodeAIRBORNE->AddChild(std::make_unique<StJump>());
    m_nodeFALL = m_nodeAIRBORNE->AddChild(std::make_unique<StFall>());

    m_nodeROOT = m_root.get();
}

void CKirby::ChangeState(KIRBY_STATE target) {
    KirbyState* to = FindNode(m_root.get(), target);
    if (!to) return;

    KirbyState* from = m_curNode;
    if (!from) {
        // 최초 진입: 경로대로 Enter
        auto path = BuildPathToRoot(to);
        for (auto* n : path) n->OnEnter(m_ctx);
        m_curNode = to;
        m_curLeaf = target;
        return;
    }

    KirbyState* lca = LCA(from, to);

    // Exit: from에서 LCA까지
    for (KirbyState* n = from; n && n != lca; n = n->parent) {
        n->OnExit(m_ctx);
    }

    // Enter: LCA→to 경로 중 공통 프리픽스 제외
    auto pathTo = BuildPathToRoot(to);
    auto pathFrom = BuildPathToRoot(from);
    size_t skip = 0;
    while (skip < pathTo.size() && skip < pathFrom.size() && pathTo[skip] == pathFrom[skip]) ++skip;
    for (size_t i = skip; i < pathTo.size(); ++i) {
        pathTo[i]->OnEnter(m_ctx);
    }

    m_curNode = to;
    m_curLeaf = target;
}

void CKirby::Update() {
    // 입력 업데이트
    if (m_input) m_input->Update();

    // 컨텍스트 채우기
    m_ctx.self = this;
    m_ctx.input = m_input.get();
    m_ctx.body = GetRigidBody();
    m_ctx.anim = GetAnimator();

    // 상태 업데이트
    if (m_curNode) {
        const float dt = CTimeMgr::GetInst()->GetfDT();
        KIRBY_STATE requested = KIRBY_STATE::END;
        m_curNode->Update(m_ctx, dt, requested);

        if (requested != KIRBY_STATE::END && requested != m_curLeaf) {
            ChangeState(requested);
        }
    }

    // 웅크리기 등으로 콜라이더 크기가 바뀔 수 있으니 보정
    UpdateColliderSize();

    // 물리/애니 갱신(각 엔진의 Update 사용)
    if (auto* rb = GetRigidBody())  rb->Update();
    if (auto* an = GetAnimator())   an->Update();
}

void CKirby::Render(HDC dc) {
    if (auto* an = GetAnimator()) {
        // 보던 방향 → 애니 반전
        an->SetFlipX(!IsFacingRight());
        an->Render(dc);
    }
    else {
        CObject::Render(dc);
    }

    // 필요 시: GetCollider()->RenderScaled(dc, 1.0f);
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
        p.y += diffY * 0.5f; // 바닥 기준으로 중심 보정
        SetPos(p);
    }
}
