#include "gamePCH.h"
#include "CDoor.h"

#include "CEventMgr.h"
#include "CCollider.h"
#include "CTimeMgr.h"
#include "CKeyMgr.h"

// 필요하면 렌더용
#include "CCamera.h"
#include "CCore.h"

CDoor::CDoor()
{
    // 그룹/타입은 프로젝트 컨벤션에 맞춰 배치
    // Door는 보통 SPECIAL 또는 TILE에 둔다. 씬에서 AddObject 시 지정해도 됨.
    // SetGroup(GROUP_TYPE::SPECIAL);

    // 문 충돌체 기본값(게임 해상도 기준 적당한 크기)
    if (!GetCollider()) {
        CreateCollider(); // CObject가 제공하는 팩토리(프로젝트에 이미 존재)
    }
    if (auto* col = GetCollider()) {
        col->SetScale(Vec2(16.f * 4.f, 32.f * 4.f)); // 64x128 정도(픽셀 4배 스케일 가정)
        col->SetOffsetPos(Vec2(0.f, 0.f));
    }
}

CDoor::~CDoor() = default;

void CDoor::ConfigureCollider(const Vec2& size, const Vec2& offset)
{
    if (auto* col = GetCollider()) {
        col->SetScale(size);
        col->SetOffsetPos(offset);
    }
}

void CDoor::Update()
{
    // 쿨다운 감소
    if (m_cooldown > 0.f) {
        m_cooldown -= CTimeMgr::GetInst()->GetfDT();
        if (m_cooldown < 0.f) m_cooldown = 0.f;
    }

    if (!m_enabled) return;

    // 플레이어가 범위 안에 있고, 재트리거 쿨다운이 끝났다면
    if (m_playerInside && m_cooldown <= 0.f)
    {
        const bool wantEnter = m_requireInput ? KEY_TAP(KEY::UP) : true;
        if (wantEnter) {
            TryFireDoorEnter();
        }
    }
}

void CDoor::Render(HDC _dc)
{
    // 문 비주얼을 별도 스프라이트로 그릴 예정이면 여기서 구현
    // 지금은 디버그용으로 충돌체만 (엔진 디버그 렌더에 맡겨도 됨)
    if (m_debugDraw && GetCollider()) {
        GetCollider()->Render(_dc);
    }
}

void CDoor::OnCollision(CCollider* other)
{
    if (!other) return;
    if (auto* owner = other->GetOwner()) {
        if (owner->GetGroup() == GROUP_TYPE::PLAYER) {
            m_playerInside = true;

            // 자동입장 모드면 OnCollision에서 바로 입장 처리도 가능하지만,
            // Update 경로로만 일원화해 유지(쿨다운/입력 체크 일관성)
        }
    }
}

void CDoor::OnCollisionExit(CCollider* other)
{
    if (!other) return;
    if (auto* owner = other->GetOwner()) {
        if (owner->GetGroup() == GROUP_TYPE::PLAYER) {
            m_playerInside = false;
        }
    }
}

void CDoor::TryFireDoorEnter()
{
    // 비활성/쿨다운 중엔 무시
    if (!m_enabled || m_cooldown > 0.f) return;

    // 이벤트 발행: DoorSystem이 받아서 페이드/목표 저장/씬전환을 처리
    tEvent e(EVENT_TYPE::DOOR_ENTER,
        (uintptr_t)this,
        (uintptr_t)m_targetScene);
    CEventMgr::GetInst()->AddEvent(e);

    // 즉시 재입장 방지
    m_cooldown = m_retriggerDelay;
    // 보통 한 번 들어가면 씬 전환되므로 비활성화까지 해도 된다.
    m_enabled = false;
}
