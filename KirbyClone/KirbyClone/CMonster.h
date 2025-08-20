#pragma once
#include "CObject.h"
#include "enum.h"

class CTexture;

class CMonster : public CObject
{
public:
    // === 정적 상수들 ===
    static constexpr float DEFAULT_SPEED = 80.f;
    static constexpr float DEFAULT_IDLE_TIME = 1.f;
    static constexpr float DAMAGE_DURATION = 0.5f;
    static constexpr float TURN_DURATION = 0.2f;

public:
    CMonster();
    virtual ~CMonster();

public:
    // === 핵심 생명주기 함수들 ===
    void Update() override;
    void Render(HDC _dc) override;

public:
    // === 충돌 콜백 함수들 ===
    void OnCollisionEnter(CCollider* _pOther) override;
    void OnCollision(CCollider* _pOther) override;
    void OnCollisionExit(CCollider* _pOther) override;

private:
    // === 충돌 처리 헬퍼 함수 ===
    void HandleTileCollision(CObject* _pTile);

public:
    // === 가상 인터페이스 (자식 클래스에서 구현) ===
    virtual void Move() = 0;                    // 이동 패턴 (순수 가상)
    virtual bool CanBeInhaled() const = 0;      // 빨아들임 가능 여부 (순수 가상)
    virtual bool HasAttack() const { return false; }   // 공격 가능 여부

protected:
    // === 애니메이션 시스템 (자식 클래스에서 사용) ===
    void LoadAnimationsFromFile(const wstring& _strFileName);   // JSON 파일에서 애니메이션 로드
    virtual void SetupAnimationMapping() = 0;                   // 자식 클래스에서 애니메이션 매핑 설정

public:
    // === 몬스터 기본 인터페이스 ===
    OBJECT_TYPE GetMonsterType() const { return GetType(); }

    // === 상태 관리 ===
    void ChangeState(MONSTER_STATE _eState);
    MONSTER_STATE GetCurrentState() const { return m_eCurState; }
    MONSTER_STATE GetPreviousState() const { return m_ePrevState; }

    // === 액션 함수들 ===
    virtual void TakeDamage();
    void TurnAround();

protected:
    // === 공통 이동 함수들 (자식 클래스에서 사용) ===
    void MoveHorizontal(float speed);          // 좌우 이동
    void HandleWallCollision();                // 벽 충돌 처리

    // === 공통 애니메이션 유틸리티 ===
    void LoadEnemySpriteSheet();               // 공통 스프라이트 시트 로드

private:
    // === 상태 업데이트 ===
    void UpdateState();
    void UpdateMove();

protected:
    // === 개별 상태 업데이트 함수들 ===
    virtual void UpdateIdle();
    virtual void UpdateWalk();
    virtual void UpdateFly();
    virtual void UpdateTurn();
    virtual void UpdateDamage();
    virtual void UpdateAttackReady();
    virtual void UpdateAttack();

public:
    // === 충돌 체크 유틸리티 ===
    bool CheckWallAhead();
    bool CheckGroundAhead();

protected:
    // === 애니메이션 매핑 (자식 클래스에서 설정) ===
    map<MONSTER_STATE, wstring> m_mapStateToAnimation;  // 상태 → 애니메이션 이름 매핑

protected:
    // === 상태 관리 ===
    MONSTER_STATE   m_eCurState;        // 현재 상태
    MONSTER_STATE   m_ePrevState;       // 이전 상태
    float           m_fStateTimer;      // 상태 타이머

protected:
    // === 이동 관련 (자식 클래스에서 접근 가능) ===
    float   m_fSpeed;           // 이동 속도
    int     m_iDir;             // 이동 방향 (-1: 왼쪽, 1: 오른쪽)
    float   m_fIdleTime;        // 대기 시간

    // === 충돌 체크 관련 ===
    bool    m_bHitWall;         // 벽 충돌 여부
    float   m_fGroundCheckDist; // 바닥 체크 거리
    float   m_fWallCheckDist;   // 벽 체크 거리

    // === 공통 텍스처 ===
    CTexture* m_pEnemyTex;      // enemies.bmp 텍스처
};