#pragma once

class CMonster;
class CPlayer;

// 몬스터 스폰 데이터 구조체
struct tMonsterSpawnData
{
    Vec2 vSpawnPos;           // 원래 생성 위치
    OBJECT_TYPE eMonsterType; // 몬스터 타입
    float fDirection;         // 방향 (-1: 왼쪽, 1: 오른쪽)
    bool bIsActive;           // 현재 활성 상태
    bool bKilled;             // 죽은 상태 (부활 방지용)
    bool bPlayerWasOutOfRange; // 플레이어가 범위 밖에 나갔는지 여부
    CMonster* pActiveMonster; // 활성화된 몬스터 포인터 (nullptr이면 비활성)
    
    tMonsterSpawnData()
        : vSpawnPos(Vec2(0.f, 0.f))
        , eMonsterType(OBJECT_TYPE::MONSTER_WADDLE_DEE)
        , fDirection(1.f)
        , bIsActive(false)
        , bKilled(false)
        , bPlayerWasOutOfRange(true)  // 처음에는 범위 밖에서 시작
        , pActiveMonster(nullptr)
    {}
    
    tMonsterSpawnData(Vec2 _vPos, OBJECT_TYPE _eType, float _fDir)
        : vSpawnPos(_vPos)
        , eMonsterType(_eType)
        , fDirection(_fDir)
        , bIsActive(false)
        , bKilled(false)
        , bPlayerWasOutOfRange(true)  // 처음에는 범위 밖에서 시작
        , pActiveMonster(nullptr)
    {}
};

class CMonsterSpawnMgr
{
    SINGLE(CMonsterSpawnMgr);

private:
    vector<tMonsterSpawnData> m_vecSpawnData;    // 스폰 데이터 목록
    CPlayer* m_pPlayer;                          // 플레이어 참조
    float m_fSpawnDistance;                      // 몬스터 활성화 거리
    float m_fDespawnDistance;                    // 몬스터 비활성화 거리
    float m_fUpdateTimer;                        // 업데이트 타이머 (최적화용)
    float m_fUpdateInterval;                     // 업데이트 간격
    int m_iMaxActiveMonsters;                    // 최대 활성 몬스터 수

public:
    // === 초기화 및 정리 ===
    void Initialize();
    void Clear();
    
    // === 스폰 데이터 관리 ===
    void AddSpawnData(Vec2 _vPos, OBJECT_TYPE _eType, float _fDirection);
    void RemoveAllSpawnData();
    size_t GetSpawnDataCount() const { return m_vecSpawnData.size(); }
    
    // === 런타임 업데이트 ===
    void Update();
    void SetPlayer(CPlayer* _pPlayer) { m_pPlayer = _pPlayer; }
    
    // === 설정 ===
    void SetSpawnDistance(float _fDistance) { m_fSpawnDistance = _fDistance; }
    void SetDespawnDistance(float _fDistance) { m_fDespawnDistance = _fDistance; }
    void SetMaxActiveMonsters(int _iMax) { m_iMaxActiveMonsters = _iMax; }
    
    float GetSpawnDistance() const { return m_fSpawnDistance; }
    float GetDespawnDistance() const { return m_fDespawnDistance; }
    int GetActiveMonsterCount() const;
    
private:
    // === 내부 로직 ===
    bool ShouldActivateMonster(const tMonsterSpawnData& _spawnData);
    bool ShouldDeactivateMonster(const tMonsterSpawnData& _spawnData);
    void ActivateMonster(tMonsterSpawnData& _spawnData);
    void DeactivateMonster(tMonsterSpawnData& _spawnData);
    CMonster* CreateMonsterInstance(OBJECT_TYPE _eType, Vec2 _vPos, float _fDirection);
    float GetDistanceToPlayer(Vec2 _vPos);
};