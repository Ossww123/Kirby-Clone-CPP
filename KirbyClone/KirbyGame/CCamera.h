#pragma once

class CCamera
{
    SINGLE(CCamera);

public:
    void init(int _iWidth, int _iHeight);
    void update();                // 위치 스무딩만 수행(외부에서 LookAt만 세팅하면 됨)
    void render(HDC _dc);         // (옵션) 디버그 마커 — 매크로로 끄고 켤 수 있게

    // === Core API ===
    void SetLookAt(Vec2 _vLook);          // 목표 지점 설정
    void SetLookAtImmediate(Vec2 _vLook); // 즉시 이동(스무딩 무시)
    Vec2 GetLookAt() const { return m_vCurLookAt; }

    void SetResolution(int w, int h) { m_vResolution = Vec2((float)w, (float)h); CalDiff(); }
    Vec2 GetResolution() const { return m_vResolution; }

    Vec2 GetRenderPos(Vec2 _vObjPos) const { return _vObjPos - m_vDiff; }
    Vec2 GetRealPos(Vec2 _vRenderPos) const { return _vRenderPos + m_vDiff; }

    // 스무딩 파라미터
    void  SetSmoothEnabled(bool v) { m_bSmooth = v; }
    void  SetSmoothSpeed(float s) { m_fSmoothSpeed = s; } // 기본 10.f
    bool  IsSmoothEnabled() const { return m_bSmooth; }
    float GetSmoothSpeed() const { return m_fSmoothSpeed; }

private:
    void CalDiff(); // 화면 중심과의 차이 계산

private:
    // === 상태 ===
    Vec2  m_vLookAt{};       // 목표(외부가 세팅)
    Vec2  m_vCurLookAt{};    // 현재(스무딩 결과)
    Vec2  m_vPrevLookAt{};
    Vec2  m_vResolution{};   // 화면 크기
    Vec2  m_vDiff{};         // 렌더 변환용 (CurLookAt - 화면중심)
    float m_fTime{ 0.f };

    // 스무딩
    bool  m_bSmooth{ true };
    float m_fSmoothSpeed{ 10.f }; // 클수록 빠르게 따라감
};
