#pragma once
#include "CObject.h"
#include "AbilityTypes.h"
#include <initializer_list>

// 전방 선언
class CTexture;
class CCollider;

// === 몬스터 공통 스탯 ==============================================
struct MonsterStats {
    int   maxHp{ 1 };
    int   hp{ 1 };
    float moveSpeed{ 80.f }; // 기본 이동속도(보행용)
    int   contactDamage{ 1 };
};

class CMonster : public CObject
{
public:
    // === 상수 타이밍들 ===
    static constexpr float DEFAULT_IDLE_TIME = 1.f;
    static constexpr float DAMAGE_DURATION = 0.5f;
    static constexpr float TURN_DURATION = 0.2f;
    static constexpr float IFAME_DURATION = 0.30f; // 피격 무적
    static constexpr float HURT_STUN_TIME = 0.10f; // 경직

public:
    CMonster();
    virtual ~CMonster();

    // === 생명주기 ===
    void Update() override;

    // === 충돌 콜백 ===
    void OnCollisionEnter(CCollider* _pOther) override;
    void OnCollision(CCollider* _pOther) override;
    void OnCollisionExit(CCollider* _pOther) override;

protected:
    // === 파생 필수 구현 ===
    virtual void Move() = 0;                     // 상태 틱에서만 호출 (속도 의도 계산)
    virtual bool CanBeInhaled() const = 0;
    virtual bool IsBeingInhaled() const = 0;
    virtual bool HasAttack() const { return false; }
    virtual bool IsBoss()    const { return false; }

    // === 애니메이션 ===
    void LoadAnimationsFromFile(const std::wstring& _strFileName);
    virtual void SetupAnimationMapping() = 0;

public:
    // === 상태 ===
    void           ChangeState(MONSTER_STATE _eState);
    MONSTER_STATE  GetCurrentState()  const { return m_eCurState; }
    MONSTER_STATE  GetPreviousState() const { return m_ePrevState; }
    float          GetStateTime()     const { return m_fStateTimer; }

    // === 전투/피격 ===
    virtual void   TakeDamage(int dmg = 1, Vec2 knockback = Vec2{ 0.f, 0.f });
    bool           IsInvincible() const { return m_fInvTime > 0.f; }

    // === 방향 ===
    bool           IsFacingRight() const { return m_iDir > 0; }
    void           SetDirection(int _iDir) { m_iDir = (_iDir >= 0 ? 1 : -1); SetFlipX(m_iDir > 0); }
    int            GetDirection() const { return m_iDir; }
    void           TurnAround(); // flipX 동기화 포함

    // === 스탯 ===
    void                 SetStats(const MonsterStats& s) { m_stats = s; }
    MonsterStats& Stats() { return m_stats; }
    const MonsterStats& Stats() const { return m_stats; }

protected:
    // === 공통 이동 헬퍼 ===
    void MoveHorizontal(float speed);

    // === 공통 리소스 ===
    void LoadEnemySpriteSheet();

    // === 애니메이션 매핑 안전 API (m_map은 private 유지) ===
    void MapAnim(MONSTER_STATE s, const std::wstring& name);
    void MapAnims(std::initializer_list<std::pair<MONSTER_STATE, const wchar_t*>> list);
    void ClearAnimMap();

private:
    // === 내부 상태 업데이트 ===
    void UpdateState();
    void UpdateIdle();
    void UpdateWalk();
    void UpdateFly();
    void UpdateTurn();
    void UpdateDamage();
    void UpdateBeingInhaled();
    void UpdateAttackReady();
    void UpdateAttack();

    // === 타이머 ===
    void UpdateTimers(float dt);

private:
    // 상태
    MONSTER_STATE m_eCurState;
    MONSTER_STATE m_ePrevState;
    float         m_fStateTimer;

    // 이동/방향
    int   m_iDir;              // -1: 왼쪽(기본), +1: 오른쪽
    float m_fIdleTime;

    // 전투/피격
    MonsterStats m_stats{};
    float        m_fInvTime{ 0.f };   // 피격 무적
    float        m_fHurtStun{ 0.f };  // 경직

    // 리소스
    CTexture* m_pEnemyTex;

    // 상태→애니메이션 이름 매핑 (private 유지)
    std::unordered_map<MONSTER_STATE, std::wstring> m_mapStateToAnimation;
};
