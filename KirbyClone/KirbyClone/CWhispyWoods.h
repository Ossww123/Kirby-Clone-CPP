#pragma once
#include "CBoss.h"

class CWhispyWoods : public CBoss
{
public:
    CWhispyWoods();
    virtual ~CWhispyWoods();

public:
    // === 가상 함수 구현 ===
    void Move() override;                                   // 고정 (이동 없음)
    
    // === 가상 함수 오버라이드 ===
    void StartBossEvent() override;                         // 보스전 시작 (위스피 우드 전용)
    
    // === 순수 가상 함수 구현 ===
    void EndBossEvent() override;                           // 보스전 종료
    void ExecuteAttackPattern(BOSS_ATTACK_PATTERN _ePattern) override;  // 공격 패턴 실행

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

public:
    // === 위스피 우드 공격 패턴들 (간소화) ===
    void AttackPattern1_AppleDrop();                        // 사과 떨어뜨리기
    void AttackPattern2_AirPuff();                          // 바람 불기

private:
    // === 공격 패턴 구현 함수들 ===
    void CreateApple(Vec2 _vPos);                           // 사과 생성
    void CreateAirPuff(Vec2 _vDirection);                   // 바람 생성
    
    // === 랜덤 공격 패턴 관리 ===
    int SelectWhispyAttackPattern();                        // 다음 공격 패턴 선택
    void ExecuteRandomAttack();                             // 랜덤 공격 실행


private:
    // === 위스피 우드 전용 변수들 ===
    float   m_fAttackTimer;                                // 공격 타이머 (통합)
    float   m_fAttackInterval;                             // 공격 간격 (3~4초 랜덤)
    
    // === 랜덤 공격 패턴 관리 ===
    int     m_iLastAttackPattern;                          // 마지막 공격 패턴
    int     m_iConsecutiveCount;                           // 연속 실행 횟수
    static const int MAX_CONSECUTIVE = 2;                  // 최대 연속 실행 횟수 (3번째 방지)
    
    // === 사과 떨어뜨리기 패턴 관리 ===
    float   m_fAppleDropTimer;                             // 사과 떨어뜨리기 내부 타이머
    int     m_iAppleDropCount;                             // 현재 떨어뜨린 사과 개수
    int     m_iAppleDropPositions[3];                      // 선택된 3개 위치 (0~4 인덱스)
    bool    m_bAppleDropInProgress;                        // 사과 떨어뜨리기 진행 중
    Vec2    m_vAppleSpawnPositions[5];                     // 5개의 사과 스폰 위치
    
    // === 공기포 발사 패턴 관리 ===
    float   m_fAirPuffTimer;                               // 공기포 발사 내부 타이머
    int     m_iAirPuffCount;                               // 현재 발사한 공기포 개수
    int     m_iTargetAirPuffCount;                         // 이번 패턴에서 발사할 총 개수 (2~4개)
    bool    m_bAirPuffInProgress;                          // 공기포 발사 진행 중
    Vec2    m_vAirPuffStartPos;                            // 공기포 발사 위치 (입 위치)
};