#include "gamePCH.h"
#include "CMonsterSpawnMgr.h"
#include "CPlayer.h"
#include "CMonster.h"
#include "CObjectFactory.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CTimeMgr.h"
#include "CRigidBody.h"

CMonsterSpawnMgr::CMonsterSpawnMgr()
    : m_pPlayer(nullptr)
    , m_fSpawnDistance(576.f)        // 커비 기준 576px에서 활성화
    , m_fDespawnDistance(768.f)      // 커비 기준 768px에서 비활성화 (히스테리시스)
    , m_fUpdateTimer(0.f)
    , m_fUpdateInterval(0.1f)        // 0.1초마다 업데이트 (성능 최적화)
    , m_iMaxActiveMonsters(10)       // 최대 10마리까지 동시 활성화
{
}

CMonsterSpawnMgr::~CMonsterSpawnMgr()
{
    Clear();
}

void CMonsterSpawnMgr::Initialize()
{
    Clear();
    m_fUpdateTimer = 0.f;
}

void CMonsterSpawnMgr::Clear()
{
    // 모든 활성 몬스터 정리
    for (auto& spawnData : m_vecSpawnData)
    {
        if (spawnData.bIsActive && spawnData.pActiveMonster)
        {
            DeactivateMonster(spawnData);
        }
        
        // 상태 초기화
        spawnData.bKilled = false;
        spawnData.bPlayerWasOutOfRange = true;
    }
    
    // 스폰 데이터 초기화
    m_vecSpawnData.clear();
    m_pPlayer = nullptr;
}

void CMonsterSpawnMgr::AddSpawnData(Vec2 _vPos, OBJECT_TYPE _eType, float _fDirection)
{
    tMonsterSpawnData spawnData(_vPos, _eType, _fDirection);
    m_vecSpawnData.push_back(spawnData);
}

void CMonsterSpawnMgr::RemoveAllSpawnData()
{
    Clear();
}

void CMonsterSpawnMgr::Update()
{
    if (!m_pPlayer)
        return;

    // 업데이트 타이머 체크 (성능 최적화)
    m_fUpdateTimer += CTimeMgr::GetInst()->GetfDT();
    if (m_fUpdateTimer < m_fUpdateInterval)
        return;
    
    m_fUpdateTimer = 0.f;

    // 현재 활성 몬스터 수 체크
    int iActiveCount = GetActiveMonsterCount();

    // 각 스폰 데이터에 대해 활성화/비활성화 판단
    for (auto& spawnData : m_vecSpawnData)
    {
        float fDistanceToPlayer = GetDistanceToPlayer(spawnData.vSpawnPos);
        bool bInRange = (fDistanceToPlayer <= m_fSpawnDistance);
        
        // 플레이어 범위 상태 업데이트
        if (!bInRange && !spawnData.bPlayerWasOutOfRange)
        {
            // 플레이어가 범위를 벗어남
            spawnData.bPlayerWasOutOfRange = true;
        }
        else if (bInRange && spawnData.bPlayerWasOutOfRange && spawnData.bKilled)
        {
            // 플레이어가 범위 밖에서 다시 들어왔고, 죽은 몬스터라면 부활 가능 상태로 리셋
            spawnData.bKilled = false;
            spawnData.bPlayerWasOutOfRange = false;
        }
        else if (bInRange)
        {
            spawnData.bPlayerWasOutOfRange = false;
        }
        
        if (!spawnData.bIsActive)
        {
            // 비활성 상태 → 활성화 조건 체크
            // 죽은 몬스터는 부활하지 않음
            if (!spawnData.bKilled && iActiveCount < m_iMaxActiveMonsters && ShouldActivateMonster(spawnData))
            {
                ActivateMonster(spawnData);
                iActiveCount++;
            }
        }
        else
        {
            // 활성 상태 → 비활성화 조건 체크
            if (ShouldDeactivateMonster(spawnData))
            {
                DeactivateMonster(spawnData);
                iActiveCount--;
            }
            // 몬스터가 죽었는지 확인
            else if (spawnData.pActiveMonster && spawnData.pActiveMonster->IsDead())
            {
                // 죽은 몬스터는 비활성화 및 죽은 상태로 마킹
                spawnData.bIsActive = false;
                spawnData.bKilled = true;  // 죽은 상태로 마킹
                spawnData.pActiveMonster = nullptr;
                iActiveCount--;
            }
        }
    }
}

int CMonsterSpawnMgr::GetActiveMonsterCount() const
{
    int iCount = 0;
    for (const auto& spawnData : m_vecSpawnData)
    {
        if (spawnData.bIsActive && spawnData.pActiveMonster && !spawnData.pActiveMonster->IsDead())
        {
            iCount++;
        }
    }
    return iCount;
}

bool CMonsterSpawnMgr::ShouldActivateMonster(const tMonsterSpawnData& _spawnData)
{
    float fDistance = GetDistanceToPlayer(_spawnData.vSpawnPos);
    return fDistance <= m_fSpawnDistance;
}

bool CMonsterSpawnMgr::ShouldDeactivateMonster(const tMonsterSpawnData& _spawnData)
{
    if (!_spawnData.pActiveMonster)
        return true;

    // 현재 몬스터 위치 기준으로 판단 (이동한 몬스터 고려)
    float fDistance = GetDistanceToPlayer(_spawnData.pActiveMonster->GetPos());
    return fDistance > m_fDespawnDistance;
}

void CMonsterSpawnMgr::ActivateMonster(tMonsterSpawnData& _spawnData)
{
    // 몬스터 인스턴스 생성
    CMonster* pMonster = CreateMonsterInstance(_spawnData.eMonsterType, _spawnData.vSpawnPos, _spawnData.fDirection);
    
    if (pMonster)
    {
        // 현재 씬에 추가
        CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
        if (pCurScene)
        {
            pCurScene->AddObject(pMonster, GROUP_TYPE::MONSTER);
        }
        
        // 스폰 데이터 업데이트
        _spawnData.bIsActive = true;
        _spawnData.pActiveMonster = pMonster;
    }
}

void CMonsterSpawnMgr::DeactivateMonster(tMonsterSpawnData& _spawnData)
{
    if (_spawnData.pActiveMonster)
    {
        // 몬스터를 Dead 상태로 만들어 씬에서 제거되도록 함
        _spawnData.pActiveMonster->SetDead();
        _spawnData.pActiveMonster = nullptr;
    }
    
    _spawnData.bIsActive = false;
}

CMonster* CMonsterSpawnMgr::CreateMonsterInstance(OBJECT_TYPE _eType, Vec2 _vPos, float _fDirection)
{
    // CObjectFactory를 통해 몬스터 생성
    CObject* pObj = CObjectFactory::CreateObject(_eType, _vPos);
    CMonster* pMonster = dynamic_cast<CMonster*>(pObj);
    
    if (pMonster)
    {
        // 게임 모드로 설정 (에디터 모드 아님)
        pMonster->SetEditorMode(false);
        
        // 방향 설정
        pMonster->SetDirection((int)_fDirection);
        
        // 리지드바디 강제 재설정 (스테이지 씬에서 문제 해결용)
        if (pMonster->GetRigidBody())
        {
            pMonster->GetRigidBody()->SetUseGravity(true);
            pMonster->GetRigidBody()->SetVelocity(Vec2(0.f, 0.f));
            pMonster->GetRigidBody()->SetGround(false);
        }
        
        // 몬스터 타입별 초기 상태 설정
        MONSTER_STATE eInitialState = MONSTER_STATE::IDLE;
        switch (_eType)
        {
        case OBJECT_TYPE::MONSTER_BRONTO_BURT:
            eInitialState = MONSTER_STATE::FLY;
            break;
        case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        case OBJECT_TYPE::MONSTER_HOT_HEAD:
            eInitialState = MONSTER_STATE::WALK;
            break;
        case OBJECT_TYPE::MONSTER_SPARKY:
        case OBJECT_TYPE::MONSTER_WHISPY_WOODS:
        default:
            eInitialState = MONSTER_STATE::IDLE;
            break;
        }
        pMonster->ChangeState(eInitialState);
    }
    
    return pMonster;
}

float CMonsterSpawnMgr::GetDistanceToPlayer(Vec2 _vPos)
{
    if (!m_pPlayer)
        return 9999.f;  // 플레이어가 없으면 매우 큰 거리 반환

    Vec2 vPlayerPos = m_pPlayer->GetPos();
    
    // X축 거리만 고려 (Y축은 무시)
    float fDistanceX = abs(_vPos.x - vPlayerPos.x);
    return fDistanceX;
}