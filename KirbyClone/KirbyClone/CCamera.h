#pragma once

class CObject;

class CCamera
{
    SINGLE(CCamera);

private:
    // === 위치 관련 ===
    Vec2        m_vLookAt;          // 카메라가 바라보는 위치 (월드 좌표)
    Vec2        m_vCurLookAt;       // 현재 카메라가 실제로 바라보는 위치
    Vec2        m_vPrevLookAt;      // 이전 프레임 카메라 위치
    Vec2        m_vDiff;            // 해상도 중앙과 카메라 LookAt 간의 차이값

    // === 타겟 추적 ===
    CObject*    m_pTargetObj;       // 카메라가 따라다닐 타겟 오브젝트

    // === 시스템 정보 ===
    Vec2        m_vResolution;      // 카메라 해상도 (화면 크기)
    float       m_fTime;            // 카메라 효과용 누적 시간

    // === 부드러운 이동 ===
    static constexpr float SMOOTH_SPEED = 10.f;     // 부드러운 이동 속도
    static constexpr float MANUAL_SPEED = 300.f;    // 수동 조작 속도

    // === 화면 흔들림 ===
    float       m_fShakeTime;       // 흔들림 지속 시간
    float       m_fShakePower;      // 흔들림 강도

public:
    void init(int _iWidth, int _iHeight);
    void update();
    void render(HDC _dc);

    // === Public Interface ===
    void SetLookAt(Vec2 _vLook) { m_vLookAt = _vLook; }
    void SetTarget(CObject* _pTarget) { m_pTargetObj = _pTarget; }
    void CameraShake(float _fDuration, float _fPower);

    // === Getters ===
    Vec2 GetLookAt() const { return m_vCurLookAt; }
    CObject* GetTarget() const { return m_pTargetObj; }
    Vec2 GetRenderPos(Vec2 _vObjPos) const { return _vObjPos - m_vDiff; }
    Vec2 GetRealPos(Vec2 _vRenderPos) const { return _vRenderPos + m_vDiff; }

private:
    // === 업데이트 함수 ===
    void UpdateTargetTracking();   // 타겟 추적
    void UpdateManualInput();      // 키보드 입력  
    void UpdateSmoothMovement();   // 부드러운 이동
    void UpdateCameraShake();      // 화면 흔들림
    void CalDiff();                // 차이값 계산
};