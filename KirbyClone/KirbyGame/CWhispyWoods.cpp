#include "gamePCH.h"
#include "CWhispyWoods.h"

#include "CTimeMgr.h"
#include "CRigidBody.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CAnimator.h"

// ------------------------------------------------------------
// 생성/기본 설정
// ------------------------------------------------------------
CWhispyWoods::CWhispyWoods()
    : CBoss([] {
    BossConfig c{};
    c.moveSpeed = 0.f;     // 기본은 제자리(원작 느낌)
    c.attackCooldown = 1.2f;    // 공격과 공격 사이 텀
    c.useGravity = false;   // 보통 고정물체처럼
    c.canTurnOnWall = false;   // 벽 자동턴 금지
    return c;
        }())
{
    // 애니메이션 로드 및 이름 매핑(시트에 맞춰 필요 시 수정)
    LoadAnimationsFromFile(L"bin/content/animation/WhispyWoods.json");
    SetAnimNames(L"IDLE", L"WALK", L"READY", L"ATTACK", L"HURT");

    // (선택) 스탯 초기화
    auto& st = Stats();
    st.maxHp = st.hp = 300;

    // (선택) 보스 크기/콜라이더 보정
    // if (auto* col = GetCollider()) col->SetScale(Vec2(96.f, 96.f));
}

// ------------------------------------------------------------
// 공격 스케줄링: 어떤 공격을 할지 결정
// ------------------------------------------------------------
BossAttackKind CWhispyWoods::ChooseNextAttack()
{
    // 기본: 열매 떨어뜨리기만 반복
    return BossAttackKind::Projectile;
}

// ------------------------------------------------------------
// READY/ATTACK 훅
// ------------------------------------------------------------
void CWhispyWoods::OnAttackReadyEnter(BossAttackKind kind)
{
    if (kind == BossAttackKind::Projectile) {
        // 공격 전 방향 정렬(연출용): 타깃이 있으면 타깃을 바라봄
        FaceTargetX();

        // 다음 공격을 위한 내부 플래그 리셋
        for (int i = 0; i < m_applesPerAttack && i < 3; ++i)
            m_spawnedFlags[i] = false;
    }
}

void CWhispyWoods::OnAttackEnter(BossAttackKind kind)
{
    if (kind == BossAttackKind::Projectile) {
        // 첫 낙하물을 바로 생성하고 싶다면 여기서
        // SpawnFallingApple(m_appleOffsets[0]);
        // m_spawnedFlags[0] = true;
    }
}

void CWhispyWoods::OnAttackTick(BossAttackKind kind, float stateTime)
{
    if (kind != BossAttackKind::Projectile) return;

    // ATTACK 상태 시간에 맞춰 좌/중/우로 낙하물 생성
    for (int i = 0; i < m_applesPerAttack && i < 3; ++i)
    {
        if (!m_spawnedFlags[i] && stateTime >= m_appleTimes[i]) {
            SpawnFallingApple(m_appleOffsets[i]);
            m_spawnedFlags[i] = true;
        }
    }
}

void CWhispyWoods::OnAttackExit(BossAttackKind /*kind*/)
{
    // 공격 종료 시 클린업이 필요하면 처리
    // (예: 히트박스/이펙트 종료 등)
}

// ------------------------------------------------------------
// 유틸/헬퍼
// ------------------------------------------------------------
void CWhispyWoods::FaceTargetX()
{
    if (!m_pTarget) return;
    const float dx = m_pTarget->GetPos().x - GetPos().x;
    const int wantDir = (dx >= 0.f) ? +1 : -1;
    if (wantDir != GetDirection())
        SetDirection(wantDir);
}

void CWhispyWoods::SpawnFallingApple(float localX)
{
    // TODO: 보스 기준 위치 + 오프셋에 낙하물(사과) 생성
    // 예시 스케치(프로젝트의 실제 클래스/스폰 방식에 맞게 교체):
    //
    // auto* apple = new CAppleProjectile();
    // apple->SetPos(GetPos() + Vec2{ localX, 20.f });
    // apple->SetInitialVelocity(Vec2{ 0.f, 0.f });  // 시작은 정지 → 중력 낙하
    // apple->SetGroup(GROUP_TYPE::PROJ_MONSTER);
    // CSceneMgr::GetInst()->GetCurScene()->AddObject(apple);
}
