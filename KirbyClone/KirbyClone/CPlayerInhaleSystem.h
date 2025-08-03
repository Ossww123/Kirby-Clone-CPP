#pragma once
#include "CObject.h"

class CPlayer;
class CRigidBody;

class CPlayerInhaleSystem
{
private:
    // === 소유자 참조 ===
    CPlayer* m_pOwner;              // 플레이어 참조

    // === 빨아들이기 관련 ===
    bool            m_bInhaling;        // 빨아들이기 중인지
    float           m_fInhaleTime;      // 빨아들이기 지속 시간
    float           m_fInhaleRange;     // 빨아들이기 범위
    Vec2            m_vInhaleDir;       // 빨아들이기 방향
    vector<CObject*> m_vecInhaleTargets; // 빨아들이기 대상들

    // === 물고 있는 것의 정보 ===
    bool            m_bHasMouthful;     // 입에 뭔가 물고 있는지
    CObject*        m_pMouthfulTarget;  // 물고 있는 것
    OBJECT_TYPE     m_eMouthfulType;    // 물고 있는 것의 타입

public:
    CPlayerInhaleSystem(CPlayer* _pOwner);
    ~CPlayerInhaleSystem();

public:
    // === 초기화 및 업데이트 ===
    void Init();
    void Update();

    // === 빨아들이기 제어 ===
    void StartInhale();
    void UpdateInhale();
    void StopInhale();

    // === 대상 관리 ===
    void UpdateInhaleTargets();
    void SwallowTarget(CObject* _pTarget);

    // === 뱉기 관련 ===
    void SpitOut();
    void ReleaseMouthful();

    // === 렌더링 ===
    void RenderInhaleEffect(HDC _dc);

    // === Getter 함수들 ===
    bool IsInhaling() const { return m_bInhaling; }
    bool HasMouthful() const { return m_bHasMouthful; }
    float GetInhaleTime() const { return m_fInhaleTime; }
    float GetInhaleRange() const { return m_fInhaleRange; }
    const Vec2& GetInhaleDir() const { return m_vInhaleDir; }
    const vector<CObject*>& GetInhaleTargets() const { return m_vecInhaleTargets; }
    CObject* GetMouthfulTarget() const { return m_pMouthfulTarget; }
    OBJECT_TYPE GetMouthfulType() const { return m_eMouthfulType; }

    // === Setter 함수들 ===
    void SetMouthful(bool _bMouthful) { m_bHasMouthful = _bMouthful; }
    void SetInhaleRange(float _fRange) { m_fInhaleRange = _fRange; }

private:
    // === 내부 헬퍼 함수들 ===
    void UpdateInhaleDirection();       // 빨아들이기 방향 업데이트
    bool IsValidInhaleTarget(CObject* _pTarget);  // 유효한 빨아들이기 대상인지 확인
    void ApplyInhaleForce(CObject* _pTarget);     // 빨아들이기 힘 적용
    Vec2 CalculateInhaleDirection();    // 현재 플레이어 방향에 따른 빨아들이기 방향 계산
};