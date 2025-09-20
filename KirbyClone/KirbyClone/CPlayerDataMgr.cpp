#include "gamePCH.h"
#include "CPlayerDataMgr.h"
#include "CPlayer.h"
#include "CPlayerHealthSystem.h"
#include "CPlayerStateMachine.h"

CPlayerDataMgr::CPlayerDataMgr()
{
}

CPlayerDataMgr::~CPlayerDataMgr()
{
}

void CPlayerDataMgr::SavePlayerState(CPlayer* _pPlayer)
{
    if (!_pPlayer)
        return;

    // 체력 정보 저장
    CPlayerHealthSystem* pHealthSystem = _pPlayer->GetHealthSystem();
    if (pHealthSystem)
    {
        m_PlayerData.iCurrentHP = pHealthSystem->GetCurrentHP();
        m_PlayerData.iMaxHP = pHealthSystem->GetMaxHP();
        m_PlayerData.bIsInvincible = pHealthSystem->IsInvincible();
        // 무적 시간은 새 씬에서 초기화하므로 저장하지 않음
        m_PlayerData.fInvincibleTime = 0.f;
    }

    // 카피 능력 저장
    CPlayerStateMachine* pStateMachine = _pPlayer->GetStateMachine();
    if (pStateMachine)
    {
        m_PlayerData.eCopyAbility = pStateMachine->GetCopyAbility();
    }

    // 현재 위치 저장 (기본값, 문에서 설정된 목적지로 덮어씀)
    m_PlayerData.vSpawnPosition = _pPlayer->GetPos();

    // 생명수는 이미 저장된 값을 유지 (게임오버가 아닌 한 변경되지 않음)

    // 데이터 유효 표시
    m_PlayerData.bHasData = true;
}

void CPlayerDataMgr::LoadPlayerState(CPlayer* _pPlayer)
{
    if (!_pPlayer || !m_PlayerData.bHasData)
        return;

    // 체력 정보 복원
    CPlayerHealthSystem* pHealthSystem = _pPlayer->GetHealthSystem();
    if (pHealthSystem)
    {
        pHealthSystem->SetHP(m_PlayerData.iCurrentHP);
        pHealthSystem->SetMaxHP(m_PlayerData.iMaxHP);
        
        // 무적 상태 복원 (단, 시간은 초기화)
        if (m_PlayerData.bIsInvincible)
        {
            pHealthSystem->StartInvincible(0.5f); // 짧은 무적 시간으로 시작
        }
    }

    // 카피 능력 복원
    CPlayerStateMachine* pStateMachine = _pPlayer->GetStateMachine();
    if (pStateMachine)
    {
        pStateMachine->SetCopyAbility(m_PlayerData.eCopyAbility);
    }

    // 위치 설정
    _pPlayer->SetPos(m_PlayerData.vSpawnPosition);
}

void CPlayerDataMgr::OnGameOver()
{
    // 생명 감소
    DecreaseLives();
    
    // 플레이어 상태 초기화 (카피 능력 상실, 체력 풀회복)
    m_PlayerData.iCurrentHP = m_PlayerData.iMaxHP;
    m_PlayerData.eCopyAbility = COPY_ABILITY::NONE;
    m_PlayerData.bIsInvincible = false;
    m_PlayerData.fInvincibleTime = 0.f;
}

bool CPlayerDataMgr::IsGameOver() const
{
    return m_PlayerData.iLives < 0;  // -1이 되어야 진짜 게임오버
}

void CPlayerDataMgr::ResetGame()
{
    // 게임 완전 초기화 (새 게임 시작)
    m_PlayerData.iLives = 2;
    m_PlayerData.iCurrentHP = 6;
    m_PlayerData.iMaxHP = 6;
    m_PlayerData.eCopyAbility = COPY_ABILITY::NONE;
    m_PlayerData.vSpawnPosition = Vec2(256.f, 384.f);
    m_PlayerData.bIsInvincible = false;
    m_PlayerData.fInvincibleTime = 0.f;
    m_PlayerData.bHasData = false;
}