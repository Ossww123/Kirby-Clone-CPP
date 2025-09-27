#pragma once
#include "CObject.h"
#include "CCollider.h"

// 보스 전투 시작을 알리는 트리거 박스(센서)
class CBossTrigger : public CObject
{
public:
    // worldLT: 좌상단, size: 가로/세로 픽셀
    CBossTrigger(const Vec2& worldLT, const Vec2& size, Vec2 lockPos = Vec2(0, 0))
        : m_active(true), m_lockPos(lockPos)
    {
        // 중심 좌표로 변환
        Vec2 center = Vec2(worldLT.x + size.x * 0.5f, worldLT.y + size.y * 0.5f);
        SetPos(center);
        SetScale(Vec2(1.f, 1.f));
        SetType(OBJECT_TYPE::TRIGGER_BOSS);
        m_size = size;
    }

    ~CBossTrigger() override = default;

    void Init() override
    {
        // 단순 AABB 센서 콜라이더
        if (!GetCollider()) CreateCollider();
        GetCollider()->SetOffsetPos(Vec2(0.f, 0.f));
        GetCollider()->SetScale(m_size);
        // TODO(sensor): CCollider에 센서 플래그가 있다면 활성화
        // GetCollider()->SetSensor(true);
    }

    void Update() override {}

    // 플레이어가 들어오면 보스전 시작 이벤트 송출
    void OnCollisionEnter(CCollider* other) override
    {
        if (!m_active || !other) return;

        // TODO(player): other->GetOwner()->GetGroup() == GROUP_TYPE::PLAYER 등으로 필터
        // 이벤트 송출 (프로젝트의 이벤트 인터페이스에 맞춰 구현)
        // 예시:
        // tEvent ev{ EVENT_TYPE::BOSS_BATTLE_START, 0, (LPARAM)this };
        // CEventMgr::GetInst()->SendEvent(ev);

        m_active = false; // 한 번만 발동
    }

    // BossSystem이 참조할 간단 API
    void SetActive(bool v) { m_active = v; }
    bool IsActive()  const { return m_active; }

    void SetLockPos(Vec2 p) { m_lockPos = p; }
    Vec2 GetLockPos() const { return (m_lockPos.x == 0.f && m_lockPos.y == 0.f) ? GetPos() : m_lockPos; }

private:
    Vec2  m_size{};
    bool  m_active{ true };
    Vec2  m_lockPos{}; // 카메라 락 포지션(미설정 시 트리거 중심)
};
