#pragma once
#include "CObject.h"

// 빨아들이기 공기 파티클 클래스
class CAirParticle : public CObject
{
public:
    CAirParticle();
    virtual ~CAirParticle();

public:
    // === 생명주기 함수 ===
    void Init();
    virtual void Update() override;
    virtual void Render(HDC _dc) override;

    // === 파티클 설정 ===
    void SetTarget(Vec2 _vTargetPos) { m_vTargetPos = _vTargetPos; }
    void SetMoveSpeed(float _fSpeed) { m_fMoveSpeed = _fSpeed; }
    void SetLifeTime(float _fLifeTime) { m_fLifeTime = _fLifeTime; }
    
    // === 상태 확인 ===
    bool IsAlive() const { return m_fTimer < m_fLifeTime; }
    bool HasReachedTarget() const;

private:
    // === 이동 관련 ===
    Vec2        m_vTargetPos;       // 목표 위치 (커비 위치)
    Vec2        m_vInitialPos;      // 초기 생성 위치
    float       m_fMoveSpeed;       // 이동 속도
    
    // === 시각 효과 ===
    float       m_fTimer;           // 생존 시간 타이머
    float       m_fLifeTime;        // 최대 생존 시간
    float       m_fBlinkTimer;      // 깜빡임 타이머
    float       m_fBlinkInterval;   // 깜빡임 간격
    bool        m_bVisible;         // 현재 보이는 상태
    
    // === 렌더링 ===
    Vec2        m_vScale;           // 파티클 크기
    COLORREF    m_Color;            // 파티클 색상 (사용 안함)
    
    // === 스프라이트 ===
    class CTexture* m_pTexture;     // kirby.bmp 텍스처
    Vec2        m_vSpritePos;       // 스프라이트 위치 (텍스처 내)

private:
    // === 내부 함수 ===
    void UpdateMovement();          // 이동 업데이트
    void UpdateBlink();             // 깜빡임 업데이트
};