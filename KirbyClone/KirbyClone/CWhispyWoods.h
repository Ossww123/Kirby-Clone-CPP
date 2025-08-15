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
    void StartBossEvent() override;                         // 보스전 시작
    void EndBossEvent() override;                           // 보스전 종료
    void ExecuteAttackPattern(BOSS_ATTACK_PATTERN _ePattern) override;  // 공격 패턴 실행

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

public:
    // === 위스피 우드 공격 패턴들 ===
    void AttackPattern1_AppleDrop();                        // 사과 떨어뜨리기
    void AttackPattern2_AirPuff();                          // 바람 불기
    void AttackPattern3_RootAttack();                       // 뿌리 공격
    void AttackPattern4_LeafStorm();                        // 잎사귀 폭풍 (2페이즈)
    void AttackPattern5_FinalAttack();                      // 최종 공격 (3페이즈)

private:
    // === 공격 패턴 구현 함수들 ===
    void CreateApple(Vec2 _vPos);                           // 사과 생성
    void CreateAirPuff(Vec2 _vDirection);                   // 바람 생성
    void CreateRoot(Vec2 _vPos);                            // 뿌리 생성
    void CreateLeaf(Vec2 _vPos, Vec2 _vDirection);          // 잎사귀 생성

    // === 보스 전용 기능들 ===
    void ShakeScreen();                                     // 화면 진동
    void CreateBossArena();                                 // 보스 전투 공간 생성
    void DestroyBossArena();                                // 보스 전투 공간 제거

private:
    // === 위스피 우드 전용 변수들 ===
    Vec2    m_vRootPositions[5];                           // 뿌리 공격 위치들
    int     m_iCurrentRootIndex;                           // 현재 뿌리 인덱스
    float   m_fAppleDropTimer;                             // 사과 떨어뜨리기 타이머
    float   m_fAirPuffTimer;                               // 바람 불기 타이머
    bool    m_bFinalPhaseStarted;                          // 최종 페이즈 시작 여부
};