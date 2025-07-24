#pragma once

class CObject;

class CCamera
{
    SINGLE(CCamera);

private:
    Vec2        m_vLookAt;          // 카메라가 바라보는 위치 (월드 좌표)
    CObject*    m_pTargetObj;       // 카메라가 따라다닐 타겟 오브젝트
    Vec2        m_vCurLookAt;       // 현재 카메라가 실제로 바라보는 위치
    Vec2        m_vPrevLookAt;      // 이전 프레임 카메라 위치

    Vec2        m_vResolution;      // 카메라 해상도 (화면 크기)
    float       m_fTime;            // 카메라 효과용 누적 시간
    float       m_fSpeed;           // 카메라 이동 속도 (보간용)
    float       m_fAccTime;         // 누적 시간
    float       m_fAccPower;        // 화면 흔들림 강도

public:
    void init(int _iWidth, int _iHeight);
    void update();
    void render(HDC _dc);

    // 카메라 위치 설정
    void SetLookAt(Vec2 _vLook) { m_vLookAt = _vLook; }
    void SetTarget(CObject* _pTarget) { m_pTargetObj = _pTarget; }

    Vec2 GetLookAt() { return m_vCurLookAt; }
    CObject* GetTarget() { return m_pTargetObj; }
    Vec2 GetRenderPos(Vec2 _vObjPos) { return _vObjPos - m_vDiff; }
    Vec2 GetRealPos(Vec2 _vRenderPos) { return _vRenderPos + m_vDiff; }

    // 화면 흔들림 효과
    void CameraShake(float _fDuration, float _fPower)
    {
        m_fAccTime = _fDuration;
        m_fAccPower = _fPower;
    }

private:
    void CalDiff();                 // 카메라와 화면 중심 간의 차이값 계산

private:
    Vec2 m_vDiff;                   // 해상도 중심점과 카메라 LookAt 간의 차이값
};