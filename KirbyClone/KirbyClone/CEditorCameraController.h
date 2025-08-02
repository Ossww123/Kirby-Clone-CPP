#pragma once

class CEditorCore;

class CEditorCameraController
{
private:
    CEditorCore* m_pEditorCore;

    // 카메라 설정
    float m_fCameraSpeed;           // 기본 이동 속도
    float m_fFastSpeed;             // 빠른 이동 속도 (Shift 누를 때)
    float m_fSlowSpeed;             // 느린 이동 속도 (Ctrl 누를 때)

    // 카메라 상태
    bool m_bCameraMoving;           // 현재 카메라가 움직이고 있는지
    Vec2 m_vLastCameraPos;          // 이전 프레임 카메라 위치

    // 카메라 제한
    bool m_bUseCameraBounds;        // 카메라 이동 범위 제한 사용
    Vec2 m_vCameraBoundsMin;        // 카메라 최소 위치
    Vec2 m_vCameraBoundsMax;        // 카메라 최대 위치

public:
    void Initialize(CEditorCore* _pCore);
    void Update();

    // 카메라 제어
    void SetCameraSpeed(float _fSpeed) { m_fCameraSpeed = _fSpeed; }
    void SetFastSpeed(float _fSpeed) { m_fFastSpeed = _fSpeed; }
    void SetSlowSpeed(float _fSpeed) { m_fSlowSpeed = _fSpeed; }
    void ResetCameraPosition();

    float GetCameraSpeed() { return m_fCameraSpeed; }
    float GetFastSpeed() { return m_fFastSpeed; }
    float GetSlowSpeed() { return m_fSlowSpeed; }

    // 카메라 위치 제어
    void SetCameraPosition(Vec2 _vPos);
    void MoveCameraBy(Vec2 _vOffset);
    void CenterCameraOn(Vec2 _vTarget);
    Vec2 GetCameraPosition();

    // 카메라 범위 제한
    void SetCameraBounds(Vec2 _vMin, Vec2 _vMax);
    void EnableCameraBounds(bool _bEnable) { m_bUseCameraBounds = _bEnable; }
    bool IsCameraBoundsEnabled() { return m_bUseCameraBounds; }

    // 카메라 상태
    bool IsCameraMoving() { return m_bCameraMoving; }

    // 편의 기능
    void ResetCameraToOrigin();
    void FocusOnPlayerSpawn();
    void FocusOnObjects();

private:
    void UpdateCameraMovement();
    void UpdateCameraState();
    void ApplyCameraBounds(Vec2& _vCameraPos);
    float GetCurrentSpeed();

public:
    CEditorCameraController();
    ~CEditorCameraController();
};