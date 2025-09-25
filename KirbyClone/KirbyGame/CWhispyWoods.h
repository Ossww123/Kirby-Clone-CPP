#pragma once
#include "CBoss.h"

// Whispy Woods 보스 틀
class CWhispyWoods : public CBoss
{
public:
    CWhispyWoods();
    ~CWhispyWoods() override = default;

    // 타깃(커비) 지정이 필요하면 사용
    void SetTarget(CObject* p) { m_pTarget = p; }

protected:
    // === 보스 훅 구현 ===
    BossAttackKind ChooseNextAttack() override;

    void OnAttackReadyEnter(BossAttackKind kind) override;
    void OnAttackEnter(BossAttackKind kind) override;
    void OnAttackTick(BossAttackKind kind, float stateTime) override;
    void OnAttackExit(BossAttackKind kind) override;

    // (필요 시) 이동/시간 커스터마이즈는 CBoss/CMonster 쪽 훅을 확장해 사용

private:
    // --- 유틸/헬퍼 (구현은 .cpp에서 TODO 처리) ---
    void FaceTargetX();
    void SpawnFallingApple(float localX); // 로컬 X 오프셋 위치에 낙하물 생성

private:
    CObject* m_pTarget{ nullptr };

    // 간단한 패턴 파라미터(원하는 값으로 조절)
    int   m_applesPerAttack = 3;
    float m_appleOffsets[3] = { -60.f, 0.f, 60.f }; // 보스 기준 좌/중/우
    float m_appleTimes[3] = { 0.20f, 0.55f, 0.90f }; // ATTACK stateTime 기준
    bool  m_spawnedFlags[3] = { false, false, false };
};
