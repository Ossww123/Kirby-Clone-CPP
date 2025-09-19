#pragma once
#include "CObject.h"

class CPlayer;
class CRigidBody;
class CAirParticle;

// === 빨아들이기 대상 추적 구조체 ===
struct InhaleTargetInfo
{
    CObject* pTarget;           // 대상 오브젝트
    Vec2 vInitialPos;          // 빨아들이기 시작 위치
    float fInhaleTimer;        // 빨아들이기 시간 (0.3초까지)

    InhaleTargetInfo ( CObject* _pTarget , Vec2 _vPos )
        : pTarget ( _pTarget ) , vInitialPos ( _vPos ) , fInhaleTimer ( 0.f )
    {}
};

class CPlayerInhaleSystem
{
private:
    // === 소유자 참조 ===
    CPlayer* m_pOwner;              // 플레이어 참조

    // === 빨아들이기 상태 ===
    bool            m_bInhaling;        // 빨아들이기 상태
    float           m_fInhaleTime;      // 빨아들이기 지속 시간
    float           m_fInhaleRange;     // 빨아들이기 범위
    Vec2            m_vInhaleDir;       // 빨아들이기 방향
    vector<InhaleTargetInfo> m_vecInhaleTargets; // 빨아들이기 대상 정보

    // === 입에 물고 있는 상태 관리 ===
    bool            m_bHasMouthful;     // 입에 물고 있는 상태
    CObject* m_pMouthfulTarget;  // 물고 있는 것
    OBJECT_TYPE     m_eMouthfulType;    // 물고 있는 것의 타입
    
    // === 파티클 시스템 ===
    vector<CAirParticle*> m_vecAirParticles;  // 공기 파티클들
    float           m_fParticleSpawnTimer;    // 파티클 생성 타이머
    float           m_fParticleSpawnInterval; // 파티클 생성 간격
    
    // === 사운드 시스템 ===
    bool            m_bInhaleSoundPlaying;    // 흡입 사운드 재생 중인지

public:
    CPlayerInhaleSystem ( CPlayer* _pOwner );
    ~CPlayerInhaleSystem ( );

public:
    // === 초기화 및 업데이트 ===
    void Init ( );
    void Update ( );

    // === 빨아들이기 관리 ===
    void StartInhale ( );
    void UpdateInhale ( );
    void StopInhale ( );

    // === 흡수 관리 ===
    void UpdateInhaleTargets ( );
    void SwallowTarget ( CObject* _pTarget );

    // === 뱉기 관리 ===
    void SpitOut ( );
    void ReleaseMouthful ( );

    // === 렌더링 ===
    void RenderInhaleEffect ( HDC _dc );
    
    // === 파티클 시스템 ===
    void UpdateParticles();         // 파티클 업데이트
    void SpawnParticles();          // 파티클 생성
    void ClearParticles();          // 모든 파티클 제거

    // === Getter 함수들 ===
    bool IsInhaling ( ) const { return m_bInhaling; }
    bool HasMouthful ( ) const { return m_bHasMouthful; }
    float GetInhaleTime ( ) const { return m_fInhaleTime; }
    float GetInhaleRange ( ) const { return m_fInhaleRange; }
    const Vec2& GetInhaleDir ( ) const { return m_vInhaleDir; }
    const vector<InhaleTargetInfo>& GetInhaleTargets ( ) const { return m_vecInhaleTargets; }
    CObject* GetMouthfulTarget ( ) const { return m_pMouthfulTarget; }
    OBJECT_TYPE GetMouthfulType ( ) const { return m_eMouthfulType; }
    
    // === 상태 확인 함수들 ===
    bool HasBeingInhaledMonsters ( ) const;  // 씬에 빨아들여지는 중인 몬스터가 있는지 확인

    // === Setter 함수들 ===
    void SetMouthful ( bool _bMouthful ) { m_bHasMouthful = _bMouthful; }
    void SetInhaleRange ( float _fRange ) { m_fInhaleRange = _fRange; }

private:
    // === 내부 처리 함수들 ===
    void UpdateInhaleDirection ( );       // 빨아들이기 방향 업데이트
    bool IsValidInhaleTarget ( CObject* _pTarget );  // 유효한 빨아들이기 대상인지 확인
    void ApplyInhaleForce ( CObject* _pTarget );     // 빨아들이기 힘 적용
    Vec2 CalculateInhaleDirection ( );    // 현재 플레이어 방향에 따른 빨아들이기 방향 계산
};