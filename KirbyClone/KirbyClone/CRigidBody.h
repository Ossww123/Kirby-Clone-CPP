#pragma once

class CObject;

class CRigidBody
{
private:
    CObject*    m_pOwner;           // 소유자 오브젝트

    Vec2        m_vVelocity;        // 속도 벡터
    Vec2        m_vAccel;           // 가속도 벡터
    Vec2        m_vForce;           // 힘 벡터 (이번 프레임에 적용될 힘)

    float       m_fMass;            // 질량
    float       m_fGravityScale;    // 중력 배율 (1.0이 기본)
    float       m_fMaxVelocity;     // 최대 속도 제한
    float       m_fFriction;        // 마찰 계수 (0~1)

    bool        m_bUseGravity;      // 중력 사용 여부
    bool        m_bGround;          // 바닥에 접촉 여부

    static float s_fGravity;        // 전역 중력값

public:
    void SetMass(float _fMass) { m_fMass = _fMass; }
    void SetGravityScale(float _fScale) { m_fGravityScale = _fScale; }
    void SetMaxVelocity(float _fMaxVel) { m_fMaxVelocity = _fMaxVel; }
    void SetFriction(float _fFriction) { m_fFriction = _fFriction; }
    void SetUseGravity(bool _bUse) { m_bUseGravity = _bUse; }
    void SetGround(bool _bGround) { m_bGround = _bGround; }
    void SetVelocity(Vec2 _vVel) { m_vVelocity = _vVel; }

    float GetMass() { return m_fMass; }
    Vec2 GetVelocity() { return m_vVelocity; }
    Vec2 GetAccel() { return m_vAccel; }
    bool IsGround() { return m_bGround; }
    bool IsUseGravity() { return m_bUseGravity; }

    // 힘 적용 함수들
    void AddForce(Vec2 _vForce);                    // 힘 추가
    void AddVelocity(Vec2 _vVelocity);              // 속도 직접 추가
    void SetVelocityX(float _fVelX);                // X 속도만 설정
    void SetVelocityY(float _fVelY);                // Y 속도만 설정

    // 물리 시뮬레이션
    void Update();

    // 전역 중력 설정
    static void SetGlobalGravity(float _fGravity) { s_fGravity = _fGravity; }
    static float GetGlobalGravity() { return s_fGravity; }

public:
    CRigidBody();
    ~CRigidBody();

    friend class CObject;
};