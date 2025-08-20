#pragma once
#include "CMonster.h"

class CInvincibleMonster : public CMonster
{
public:
    CInvincibleMonster();
    virtual ~CInvincibleMonster();

public:
    // === 무적 관련 (final로 하위 클래스에서 변경 불가) ===
    bool CanBeInhaled() const override final { return false; }

public:
    // === 충돌 콜백 오버라이드 ===
    void OnCollisionEnter(CCollider* _pOther) override;
    void OnCollision(CCollider* _pOther) override;
    void OnCollisionExit(CCollider* _pOther) override;

protected:
    // === 무적 몬스터 공통 기능 ===
    void HandleInvincibleCollision(CCollider* _pOther);     // 무적 충돌 처리
    void PushAwayPlayer(CCollider* _pOther);                // 플레이어 밀어내기
    void CreateInvincibleEffect();                          // 무적 이펙트 생성

    // === 데미지 무효화 ===
    void TakeDamage() override;                             // 데미지 무효화

protected:
    // === 무적 상태 관리 ===
    void ShowInvincibleFeedback();                          // 무적 피드백 표시
    void PlayInvincibleSound();                             // 무적 사운드 재생

private:
    // === 이펙트 관련 ===
    float   m_fEffectTimer;                                 // 이펙트 타이머
    bool    m_bShowingEffect;                               // 이펙트 표시 중
};