#pragma once

class CGrid
{
    SINGLE(CGrid);

private:
    float   m_fGridSize;        // 그리드 크기 (픽셀 단위)
    bool    m_bShowGrid;        // 그리드 표시 여부
    bool    m_bSnapToGrid;      // 그리드 스냅 활성화
    Vec2    m_vGridOffset;      // 그리드 오프셋 (카메라 이동 시 정렬용)

    // 그리드 렌더링 색상
    COLORREF m_gridColor;
    COLORREF m_majorGridColor;

public:
    void init();
    void Render(HDC _dc);

    // 그리드 설정
    void SetGridSize(float _fSize) { m_fGridSize = _fSize; }
    void SetShowGrid(bool _bShow) { m_bShowGrid = _bShow; }
    void SetSnapToGrid(bool _bSnap) { m_bSnapToGrid = _bSnap; }
    void SetGridOffset(Vec2 _vOffset) { m_vGridOffset = _vOffset; }

    // 그리드 정보 조회
    float GetGridSize() { return m_fGridSize; }
    bool IsShowGrid() { return m_bShowGrid; }
    bool IsSnapToGrid() { return m_bSnapToGrid; }

    // 그리드 스냅 기능
    Vec2 SnapToGrid(Vec2 _vPos);                    // 위치를 가장 가까운 그리드에 맞춤
    Vec2 GetGridPosition(Vec2 _vWorldPos);          // 월드 좌표를 그리드 좌표로 변환
    Vec2 GetWorldPosition(Vec2 _vGridPos);          // 그리드 좌표를 월드 좌표로 변환

    // 그리드 범위 계산 (화면에 보이는 그리드만 렌더링하기 위함)
    void GetVisibleGridRange(Vec2& _vStart, Vec2& _vEnd);

    // 그리드 크기 프리셋
    void SetGridSizePreset(int _iPreset);           // 1: 32px, 2: 64px, 3: 128px, 4: 256px
};