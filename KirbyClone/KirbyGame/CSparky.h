#pragma once
#include "CCopyMonster.h"

class CSparky : public CCopyMonster
{
public:
    CSparky();
    virtual ~CSparky();

private:
    // === 스파키 점프 타입 ===
    enum class SPARKY_JUMP_TYPE
    {
        SMALL_IN_PLACE ,     // 제자리 작은 점프 (y: 0.5타일)
        SMALL_FORWARD ,      // 작은 전진 점프 (x: 1타일, y: 0.5타일)
        BIG_FORWARD ,        // 큰 전진 점프 (x: 2타일, y: 1.2타일)
        END
    };

    // === 스파키 상태 ===
    enum class SPARKY_STATE
    {
        IDLE ,               // 대기 상태
        JUMP ,               // 점프 상태
        END
    };

public:
    // === 가상 함수 구현 ===
    void Move() override;                                           // 점프 + 전기 공격
    void Attack() override;                                         // 전기 공격
    void UpdateAttackReady() override;                              // 공격 준비 상태 업데이트
    void UpdateAttack() override;                                   // 공격 상태 업데이트
    void EndAttack() override;                                      // 공격 종료 후 IDLE로 복귀
    COPY_ABILITY GetCopyAbility() const override { return COPY_ABILITY::SPARK; }

protected:
    // === 애니메이션 매핑 설정 구현 ===
    void SetupAnimationMapping() override;

    // === 상태 업데이트 오버라이드 ===
    void UpdateIdle() override;
    void UpdateWalk() override;

private:
    // === 스파키 상태 관리 ===
    void UpdateSparkyState();                                       // 스파키 상태 업데이트
    void UpdateSparkyIdle();                                        // 스파키 IDLE 상태 업데이트
    void UpdateSparkyJump();                                        // 스파키 JUMP 상태 업데이트

    // === 점프 시스템 ===
    void StartJump();                                               // 점프 시작
    SPARKY_JUMP_TYPE SelectRandomJumpType();                       // 랜덤 점프 타입 선택
    void CalculateJumpTarget();                                     // 점프 목표 계산
    Vec2 CalculateParabolicPosition(float progress);                // 포물선 위치 계산
    bool CheckJumpCollision(const Vec2& targetPos);                 // 점프 중 충돌 검사
    float FindGroundHeight(float xPos);                             // 특정 X 위치에서 바닥 높이 찾기

    // === 커비 추적 ===
    void UpdateKirbyDirection();                                    // 커비 방향 업데이트
    void UpdatePlayerPosition();                                    // 플레이어 위치 업데이트
    float GetTileSize() const { return 64.f; }                     // 타일 크기 반환

    // === 스파키 전용 공격 ===
    void CreateElectricField();                                     // 전기장 생성
    void CreateElectricSpark();                                     // 전기 스파크 생성

private:
    // === 상태 관리 ===
    SPARKY_STATE        m_eSparkyState;                             // 스파키 고유 상태
    float               m_fIdleTimer;                               // IDLE 대기 시간
    float               m_fIdleDuration;                            // IDLE 지속 시간 (랜덤)

    // === 점프 관련 ===
    SPARKY_JUMP_TYPE    m_eJumpType;                                // 현재 점프 타입
    bool                m_bJumping;                                 // 점프 중 상태
    Vec2                m_vJumpStartPos;                            // 점프 시작 위치
    Vec2                m_vJumpTargetPos;                           // 점프 목표 위치
    float               m_fJumpProgress;                            // 점프 진행도 (0~1)
    float               m_fJumpDuration;                            // 점프 지속 시간

    // === 커비 추적 ===
    Vec2                m_vKirbyDirection;                          // 커비 방향 벡터

    // === 공격 관련 ===
    bool                m_bElectricFieldCreated;                    // 전기장 생성 여부
    float               m_fSoundTimer;                              // 사운드 반복 타이머
    float               m_fSoundInterval;                           // 사운드 반복 간격
};