#pragma once
#include <cstdint>

class CKirby;
class CRigidBody;
class CAnimator;

class CKirbyHealthSystem {
public:
    explicit CKirbyHealthSystem(CKirby* owner);
    ~CKirbyHealthSystem();

    void Update(float dt);

    // 직접 호출 버전(이벤트를 쓰지 않는다면 이걸 사용)
    void TakeDamage(int dmg, const Vec2& knockDir);
    void Heal(int amount);

    // 쿼리
    bool IsAlive() const { return m_hp > 0; }
    bool IsInvincible() const { return m_invincibleTime > 0.f; }
    int  GetHP() const { return m_hp; }
    int  GetMaxHP() const { return m_maxHp; }

    // 설정
    void SetMaxHP(int v) { m_maxHp = v; if (m_hp > m_maxHp) m_hp = m_maxHp; }
    void SetInvincibleDuration(float sec) { m_invincibleDuration = sec; }
    void SetKnockbackPower(float p) { m_knockPower = p; }

    // 리스폰/초기화용
    void SetHP(int v) { m_hp = std::clamp(v, 0, m_maxHp); }
    void StartInvincible(float sec) { m_invincibleTime = (std::max)(0.f, sec); }
    void ClearInvincibility() { m_invincibleTime = 0.f; }

private:
    void OnDeath();
    void ApplyKnockback(const Vec2& dir);

private:
    CKirby* m_owner{ nullptr };
    int     m_maxHp{ 6 };
    int     m_hp{ 6 };

    float   m_invincibleTime{ 0.f };
    float   m_invincibleDuration{ 1.0f };
    float   m_knockPower{ 350.f };

    // (선택) 점멸 표현 등에 쓰고 싶다면
    float   m_blinkTimer{ 0.f };
};
