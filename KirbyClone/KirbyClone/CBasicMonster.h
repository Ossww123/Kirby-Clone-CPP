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
    virtual void OnInhaleStart();           // 빨아들임 시작\

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
    void Update() override;             // 메인 업데이트 (죽음 효과 처리 포함)
    void UpdateIdle() override;
    void UpdateWalk() override;
    void UpdateFly() override;          // 비행 상태도 추가
    void UpdateTurn() override;

public:
    // === 감지 관련 Getter/Setter ===
    bool IsPlayerDetected() const { return m_bPlayerDetected; }
    float GetDetectionRange() const { return m_fDetectionRange; }
    void SetDetectionRange(float _fRange) { m_fDetectionRange = _fRange; }
    const Vec2& GetPlayerPos() const { return m_vPlayerPos; }

protected:
    // === 플레이어 감지 관련 ===
    bool    m_bPlayerDetected;      // 플레이어 감지 여부
    float   m_fDetectionRange;      // 플레이어 감지 범위
    Vec2    m_vPlayerPos;           // 플레이어 위치

    // === 빨아들임 관련 내부 처리 ===
    void ProcessInhaleMovement();           // 빨아들임 중 이동 처리
    void CheckInhaleDistance();             // 빨아들임 거리 체크

protected:
    // === 빨아들임 상태 ===
    bool    m_bBeingInhaled;        // 빨아들임 중 상태
    float   m_fInhaleForce;         // 빨아들임 힘

    // === 체력 시스템 ===
    int     m_iHealth;              // 현재 체력
    int     m_iMaxHealth;           // 최대 체력

    // === 죽음 효과 관련 ===
    bool    m_bIsDying;             // 죽는 중인지 여부
    float   m_fDeathEffectTimer;    // 죽음 효과 타이머
    Vec2    m_vKnockbackDir;        // 넉백 방향
    float   m_fKnockbackSpeed;      // 넉백 속도

public:
    // === 체력 관련 함수 ===
    int GetHealth() const { return m_iHealth; }
    int GetMaxHealth() const { return m_iMaxHealth; }
    void SetHealth(int _iHealth) { m_iHealth = _iHealth; }
    bool IsDying() const { return m_bIsDying; }
    
    // === 죽음 효과 처리 ===
    void StartDeathEffect(Vec2 _vKnockbackDir);
    void UpdateDeathEffect();

    // === 데미지 처리 오버라이드 ===
    void TakeDamage() override;
    
    // === 데미지 소스 위치 설정 (이벤트 시스템용) ===
    void SetDamageSourcePos(Vec2 _vPos) { m_vPlayerPos = _vPos; }
};
