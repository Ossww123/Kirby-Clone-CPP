#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include "CObject.h"

// 전방 선언
class CAnimator;
class CRigidBody;
class CCollider;
class CPlayerInputManager;
class CKirbyMovement;
class CKirbyHealthSystem;

// ===========================
// KIRBY HFSM 노드 ID
// ===========================
enum class KIRBY_STATE : uint16_t {
    ROOT = 0,

    // 공통 슈퍼상태
    GROUNDED,
    IDLE, WALK, RUN, CROUCH,
    AIRBORNE,
    JUMP, FALL,

    // 이후 확장 트리(지금은 미구현)
    INHALE_TREE, INHALE, INHALE_KEEP, SWALLOW, EXHALE,
    MOUTHFUL_TREE, MOUTHFUL_IDLE, MOUTHFUL_WALK, MOUTHFUL_RUN, MOUTHFUL_JUMP, MOUTHFUL_FALL,
    COMBAT_TREE, ATTACK, ATTACK_HOLD,
    DAMAGED_TREE, DAMAGE,
    META_TREE, GAMEOVER, VICTORY,

    END
};

// ===========================
// HFSM 컨텍스트
// ===========================
struct KirbyStateCtx {
    class CKirby* self{ nullptr };
    CPlayerInputManager* input{ nullptr };
    CRigidBody* body{ nullptr };
    CAnimator* anim{ nullptr };
    CKirbyMovement* move{ nullptr };
};

// ===========================
// HFSM 상태 베이스
// ===========================
class KirbyState {
public:
    explicit KirbyState(KIRBY_STATE id, KirbyState* parent = nullptr)
        : id(id), parent(parent) {}
    virtual ~KirbyState() = default;

    KIRBY_STATE id;
    KirbyState* parent{ nullptr };
    std::vector<std::unique_ptr<KirbyState>> children;

    virtual void OnEnter(KirbyStateCtx&) {}
    virtual void OnExit(KirbyStateCtx&) {}
    // Update에서 다른 리프 상태로 전환을 원하면 outRequested에 목표 leaf를 기록
    virtual void Update(KirbyStateCtx&, float dt, KIRBY_STATE& outRequested) { (void)dt; (void)outRequested; }

    KirbyState* AddChild(std::unique_ptr<KirbyState> ch) {
        KirbyState* raw = ch.get();
        ch->parent = this;
        children.push_back(std::move(ch));
        return raw;
    }
};

// ===========================
// CKirby 본체
// ===========================
class CKirby : public CObject {
public:
    CKirby();
    ~CKirby() override;

    // 생명주기
    void Update() override;
    void Render(HDC dc) override;

    // 충돌
    void OnCollisionEnter(CCollider* other) override;
    void OnCollision(CCollider* other) override;
    void OnCollisionExit(CCollider* other) override;

    // 방향
    bool IsFacingRight() const { return m_bFacingRight; }
    void SetFacingRight(bool r) { m_bFacingRight = r; }

    // 상태 조회
    KIRBY_STATE GetCurrentLeaf() const { return m_curLeaf; }

    // 입력 매니저
    CPlayerInputManager* GetInput() const { return m_input.get(); }
    CKirbyMovement* GetMovement() const { return m_move.get(); }
    CKirbyHealthSystem* GetHealth() const { return m_health.get(); }
    CKirbyHealthSystem* GetHealthSystem() const { return m_health.get(); }

    // 애니메이션 로딩
    void LoadDefaultAnimations();
    void LoadAbilityAnimations(int /*abilityId*/);

    void DoSlideKickRecoil();

    // Lives API
    int  GetLives() const { return m_lives; }
    void SetLives(int v) { m_lives = v; }
    void AddLife(int v = 1) { m_lives += v; }
    void DecLife(int v = 1) { m_lives -= v; }


    // 리스폰 시 초기화(HP 풀회복, 무적 해제/초기 무적 등)
    void ResetForRespawn(bool briefInvincible = true);
private:
    // HFSM
    void BuildHFSM();
    void ChangeState(KIRBY_STATE target);

    static std::vector<KirbyState*> BuildPathToRoot(KirbyState* node);
    static KirbyState* LCA(KirbyState* a, KirbyState* b);
    static KirbyState* FindNode(KirbyState* node, KIRBY_STATE id);

    // 충돌체 유틸
    void UpdateColliderSize();
    void AdjustPositionForColliderResize(const Vec2& oldScale, const Vec2& newScale);

private:
    // 입력
    std::unique_ptr<CPlayerInputManager> m_input;
    std::unique_ptr<CKirbyMovement>     m_move;
    std::unique_ptr<CKirbyHealthSystem> m_health;

    // HFSM 트리
    std::unique_ptr<KirbyState> m_root;
    KirbyState* m_nodeROOT{ nullptr };
    KirbyState* m_nodeGROUNDED{ nullptr };
    KirbyState* m_nodeIDLE{ nullptr };
    KirbyState* m_nodeWALK{ nullptr };
    KirbyState* m_nodeRUN{ nullptr };
    KirbyState* m_nodeCROUCH{ nullptr };
    KirbyState* m_nodeAIRBORNE{ nullptr };
    KirbyState* m_nodeJUMP{ nullptr };
    KirbyState* m_nodeFALL{ nullptr };

    KirbyState* m_curNode{ nullptr };
    KIRBY_STATE m_curLeaf{ KIRBY_STATE::IDLE };

    bool m_bFacingRight{ true };

    // 충돌체 기본/웅크리기 사이즈
    Vec2 m_vNormalCollider{ 56.f, 56.f };
    Vec2 m_vCrouchCollider{ 56.f, 28.f };

    // 상태 업데이트 컨텍스트
    KirbyStateCtx m_ctx;

    int  m_lives{ 2 };
};
