#pragma once
#include "CMonster.h"

class CBasicMonster : public CMonster
{
public:
    CBasicMonster();
    virtual ~CBasicMonster();

public:
    // === 빨아들임 관련 (final로 하위 클래스에서 변경 불가) ===
    bool CanBeInhaled() const override final { return true; }

    // === 빨아들임 처리 ===
    virtual void OnInhaled();               // 빨아들임 당했을 때 처리
    virtual void OnInhaleStart();           // 빨아들임 시작
    virtual void OnInhaleEnd();             // 빨아들임 종료

public:
    // === 빨아들임 상태 확인 ===
    bool IsBeingInhaled() const { return m_bBeingInhaled; }
    void SetInhaled(bool _bInhaled) { m_bBeingInhaled = _bInhaled; }

protected:
    // === 일반 몬스터 공통 기능들 ===
    void CheckPlayerDistance();             // 플레이어와의 거리 체크
    void HandleInhaleEffect();              // 빨아들임 이펙트 처리
    void UpdateInhaleState();               // 빨아들임 상태 업데이트

    // === 일반 몬스터 공통 상태 업데이트 ===
    void UpdateIdle() override;
    void UpdateWalk() override;
    void UpdateFly() override;          // 비행 상태도 추가
    void UpdateTurn() override;

private:
    // === 빨아들임 관련 내부 처리 ===
    void ProcessInhaleMovement();           // 빨아들임 중 이동 처리
    void CheckInhaleDistance();             // 빨아들임 거리 체크

private:
    // === 빨아들임 상태 ===
    bool    m_bBeingInhaled;        // 빨아들임 중 상태
    float   m_fInhaleForce;         // 빨아들임 힘
    Vec2    m_vPlayerPos;           // 플레이어 위치 (빨아들임용)
    float   m_fInhaleDistance;      // 빨아들임 가능 거리
};
