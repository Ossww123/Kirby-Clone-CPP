#pragma once

class CEditorCore;

class CEditorCameraController
{
public:
    // === 생성자 함수 ===
    CEditorCameraController();
    ~CEditorCameraController();

public:
    // === 메인 진입점 함수 ===
    void Initialize(CEditorCore* _pCore);
    void Update();

private:
    // === 카메라 업데이트 내부 처리 ===
    void UpdateCameraMovement();
    void UpdateCameraState();
    void ProcessKeyboardInput(Vec2& _vMoveDir, bool& _bInputDetected);

public:
    // === 카메라 속도 설정 ===
    void SetCameraSpeed(float _fSpeed) { m_fCameraSpeed = _fSpeed; }
    void SetFastSpeed(float _fSpeed) { m_fFastSpeed = _fSpeed; }
    void SetSlowSpeed(float _fSpeed) { m_fSlowSpeed = _fSpeed; }

private:
    // === 속도 계산 관련 함수 ===
    float GetCurrentSpeed();

public:
    // === 카메라 위치 제어 ===
    void SetCameraPosition(Vec2 _vPos);
    void MoveCameraBy(Vec2 _vOffset);
    void CenterCameraOn(Vec2 _vTarget);
    void ResetCameraPosition();
    void ResetCameraToOrigin();

public:
    // === 카메라 경계 설정 ===
    void SetCameraBounds(Vec2 _vMin, Vec2 _vMax);

private:
    // === 경계 범위 제한 처리 ===
    void ApplyCameraBounds(Vec2& _vCameraPos);

public:
    // === Getter �Լ��� ===
    float GetCameraSpeed() const { return m_fCameraSpeed; }
    float GetFastSpeed() const { return m_fFastSpeed; }
    float GetSlowSpeed() const { return m_fSlowSpeed; }

    Vec2 GetCameraPosition() const;

    Vec2 GetCameraBoundsMin() const { return m_vCameraBoundsMin; }
    Vec2 GetCameraBoundsMax() const { return m_vCameraBoundsMax; }

    bool IsCameraMoving() const { return m_bCameraMoving; }

private:
    // === 멤버 변수들 ===

    // === 에디터 코어 참조 ===
    CEditorCore* m_pEditorCore;     // 에디터 코어 참조

    // === 카메라 속도 ===
    float       m_fCameraSpeed;     // 기본 이동 속도
    float       m_fFastSpeed;       // 빠른 이동 속도 (Shift 누를 때)
    float       m_fSlowSpeed;       // 느린 이동 속도 (Ctrl 누를 때)

    // === 카메라 상태 ===
    bool        m_bCameraMoving;    // 현재 카메라 움직이고 있는지
    Vec2        m_vLastCameraPos;   // 이전 프레임의 카메라 위치

    // === 카메라 경계 ===
    Vec2        m_vCameraBoundsMin; // 카메라 최소 위치
    Vec2        m_vCameraBoundsMax; // 카메라 최대 위치
};