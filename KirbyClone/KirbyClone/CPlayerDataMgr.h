#pragma once
#include "pch.h"

// 플레이어 상태 보존용 구조체
struct tPlayerData
{
    int iCurrentHP;              // 현재 체력
    int iMaxHP;                  // 최대 체력
    COPY_ABILITY eCopyAbility;   // 카피 능력
    Vec2 vSpawnPosition;         // 스폰 위치
    bool bIsInvincible;          // 무적 상태
    float fInvincibleTime;       // 남은 무적 시간
    bool bHasData;               // 유효한 데이터인지 확인
    int iLives;                  // 생명 수

    tPlayerData()
        : iCurrentHP(6)
        , iMaxHP(6)
        , eCopyAbility(COPY_ABILITY::NONE)
        , vSpawnPosition(Vec2(256.f, 384.f))
        , bIsInvincible(false)
        , fInvincibleTime(0.f)
        , bHasData(false)
        , iLives(2)                  // 기본 생명 수 2
    {
    }
};

class CPlayer;

class CPlayerDataMgr
{
    SINGLE(CPlayerDataMgr);

private:
    tPlayerData m_PlayerData;    // 임시 저장된 플레이어 데이터

public:
    // === 상태 저장/로드 ===
    void SavePlayerState(CPlayer* _pPlayer);
    void LoadPlayerState(CPlayer* _pPlayer);
    
    // === 위치 설정 ===
    void SetSpawnPosition(Vec2 _vPos) { m_PlayerData.vSpawnPosition = _vPos; }
    Vec2 GetSpawnPosition() const { return m_PlayerData.vSpawnPosition; }
    
    
    // === 생명 수 관리 ===
    int GetLives() const { return m_PlayerData.iLives; }
    void SetLives(int _iLives) { m_PlayerData.iLives = _iLives; }
    void DecreaseLives() { m_PlayerData.iLives--; }  // 음수까지 허용
    bool HasLivesLeft() const { return m_PlayerData.iLives > 0; }
    
    // === 게임오버 관련 ===
    void OnGameOver();          // 게임오버 처리 (생명 감소)
    bool IsGameOver() const;    // 완전 게임오버 확인
    void ResetGame();           // 게임 초기화 (생명수 리셋)
    
    // === 데이터 확인 ===
    bool HasSavedData() const { return m_PlayerData.bHasData; }
    void ClearSavedData() { m_PlayerData.bHasData = false; }
    
    // === 디버그 ===
    const tPlayerData& GetSavedData() const { return m_PlayerData; }
};