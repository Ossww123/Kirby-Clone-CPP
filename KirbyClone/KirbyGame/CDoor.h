#pragma once
#include "CObject.h"

// 문 오브젝트: 충돌 범위 + 간단한 입력만 처리. 나머지는 시스템에서 처리.
class CDoor : public CObject
{
public:
    CDoor();
    ~CDoor() override;

    // --- CObject 기본 수명주기 ---
    void Update() override;
    void Render(HDC _dc) override;

    // --- 충돌 콜백 ---
    void OnCollision(CCollider* _pOther) override;
    void OnCollisionExit(CCollider* _pOther) override;

public:
    // --- 문 설정 ---
    void SetTargetScene(SCENE_TYPE sc) { m_targetScene = sc; }
    SCENE_TYPE GetTargetScene() const { return m_targetScene; }

    void SetTargetPosition(const Vec2& pos) { m_targetSpawn = pos; }
    Vec2 GetTargetPosition() const { return m_targetSpawn; }

    // 플레이어가 범위 안에서 ↑ 입력을 눌러야 들어가는지(기본: true)
    void SetRequireInput(bool v) { m_requireInput = v; }
    bool GetRequireInput() const { return m_requireInput; }

    // 자동 입장(=Overlap 즉시 입장) 켜기/끄기
    void SetAutoEnterOnOverlap(bool v) { m_requireInput = !v; }

    // 활성/비활성
    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    // 충돌체 설정(편의): 크기/오프셋 (월드 단위)
    void ConfigureCollider(const Vec2& size, const Vec2& offset = Vec2(0.f, 0.f));

    // 재트리거 지연(초) 설정
    void SetRetriggerDelay(float sec) { m_retriggerDelay = (std::max)(0.f, sec); }

private:
    // 입장 이벤트 발행 (한 곳에서만 호출)
    void TryFireDoorEnter();

private:
    // 목적지
    SCENE_TYPE m_targetScene{ SCENE_TYPE::START };
    Vec2       m_targetSpawn{ 256.f, 384.f };

    // 상태
    bool  m_enabled{ true };
    bool  m_playerInside{ false };
    bool  m_requireInput{ true };   // true면 ↑ 입력 필요, false면 자동
    float m_cooldown{ 0.f };
    float m_retriggerDelay{ 0.5f }; // 중복 트리거 방지

    // 디버그 표시
    bool  m_debugDraw{ false };
};
