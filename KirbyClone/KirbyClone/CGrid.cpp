#include "pch.h"
#include "CGrid.h"
#include "CCamera.h"
#include "CCore.h"
#include <cmath>

CGrid::CGrid()
    : m_fGridSize(64.f)
    , m_bShowGrid(true)
    , m_bSnapToGrid(true)
    , m_vGridOffset{}
    , m_gridColor(RGB(80, 80, 80))
    , m_majorGridColor(RGB(120, 120, 120))
{
}

CGrid::~CGrid()
{
}

void CGrid::init()
{
    // 기본 그리드 설정
    m_fGridSize = 64.f;
    m_bShowGrid = true;
    m_bSnapToGrid = true;
    m_vGridOffset = Vec2(0.f, 0.f);
}

void CGrid::Render(HDC _dc)
{
    if (!m_bShowGrid)
        return;

    // 화면 해상도 가져오기
    Vec2 vResolution = CCore::GetInst()->GetResolution();

    // 카메라 위치 가져오기
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    // 화면에 보이는 영역의 월드 좌표 계산
    Vec2 vScreenStart = vCameraPos - vResolution / 2.f;
    Vec2 vScreenEnd = vCameraPos + vResolution / 2.f;

    // 그리드 시작/끝 위치 계산 (그리드에 맞춰 정렬)
    int iStartX = (int)(vScreenStart.x / m_fGridSize) - 1;
    int iStartY = (int)(vScreenStart.y / m_fGridSize) - 1;
    int iEndX = (int)(vScreenEnd.x / m_fGridSize) + 1;
    int iEndY = (int)(vScreenEnd.y / m_fGridSize) + 1;

    // 그리드 펜 설정
    HPEN hGridPen = CreatePen(PS_SOLID, 1, m_gridColor);
    HPEN hMajorGridPen = CreatePen(PS_SOLID, 1, m_majorGridColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hGridPen);

    // 세로선 그리기
    for (int x = iStartX; x <= iEndX; ++x)
    {
        float fWorldX = x * m_fGridSize + m_vGridOffset.x;
        Vec2 vLineStart = Vec2(fWorldX, vScreenStart.y);
        Vec2 vLineEnd = Vec2(fWorldX, vScreenEnd.y);

        // 월드 좌표를 화면 좌표로 변환
        Vec2 vRenderStart = CCamera::GetInst()->GetRenderPos(vLineStart);
        Vec2 vRenderEnd = CCamera::GetInst()->GetRenderPos(vLineEnd);

        // 주요 그리드선 (5배수)인지 확인
        if (x % 5 == 0)
        {
            SelectObject(_dc, hMajorGridPen);
        }
        else
        {
            SelectObject(_dc, hGridPen);
        }

        // 선 그리기
        MoveToEx(_dc, (int)vRenderStart.x, (int)vRenderStart.y, nullptr);
        LineTo(_dc, (int)vRenderEnd.x, (int)vRenderEnd.y);
    }

    // 가로선 그리기
    for (int y = iStartY; y <= iEndY; ++y)
    {
        float fWorldY = y * m_fGridSize + m_vGridOffset.y;
        Vec2 vLineStart = Vec2(vScreenStart.x, fWorldY);
        Vec2 vLineEnd = Vec2(vScreenEnd.x, fWorldY);

        // 월드 좌표를 화면 좌표로 변환
        Vec2 vRenderStart = CCamera::GetInst()->GetRenderPos(vLineStart);
        Vec2 vRenderEnd = CCamera::GetInst()->GetRenderPos(vLineEnd);

        // 주요 그리드선 (5배수)인지 확인
        if (y % 5 == 0)
        {
            SelectObject(_dc, hMajorGridPen);
        }
        else
        {
            SelectObject(_dc, hGridPen);
        }

        // 선 그리기
        MoveToEx(_dc, (int)vRenderStart.x, (int)vRenderStart.y, nullptr);
        LineTo(_dc, (int)vRenderEnd.x, (int)vRenderEnd.y);
    }

    // 펜 복원
    SelectObject(_dc, hOldPen);
    DeleteObject(hGridPen);
    DeleteObject(hMajorGridPen);
}

Vec2 CGrid::SnapToGrid(Vec2 _vPos)
{
    if (!m_bSnapToGrid)
        return _vPos;

    // 모든 오브젝트가 그리드 셀의 중심에 배치되도록 수정
    float fGridX = floor((_vPos.x - m_vGridOffset.x) / m_fGridSize);
    float fGridY = floor((_vPos.y - m_vGridOffset.y) / m_fGridSize);

    // 그리드 중심 + (그리드 크기 / 2) = 셀의 중심
    float fSnappedX = fGridX * m_fGridSize + m_vGridOffset.x + (m_fGridSize / 2.f);
    float fSnappedY = fGridY * m_fGridSize + m_vGridOffset.y + (m_fGridSize / 2.f);

    return Vec2(fSnappedX, fSnappedY);
}

Vec2 CGrid::GetGridPosition(Vec2 _vWorldPos)
{
    // 월드 좌표를 그리드 인덱스로 변환
    float fGridX = (_vWorldPos.x - m_vGridOffset.x) / m_fGridSize;
    float fGridY = (_vWorldPos.y - m_vGridOffset.y) / m_fGridSize;

    return Vec2(floor(fGridX), floor(fGridY));
}

Vec2 CGrid::GetWorldPosition(Vec2 _vGridPos)
{
    // 그리드 인덱스를 월드 좌표로 변환
    float fWorldX = _vGridPos.x * m_fGridSize + m_vGridOffset.x;
    float fWorldY = _vGridPos.y * m_fGridSize + m_vGridOffset.y;

    return Vec2(fWorldX, fWorldY);
}

void CGrid::GetVisibleGridRange(Vec2& _vStart, Vec2& _vEnd)
{
    // 화면 해상도와 카메라 위치로 보이는 그리드 범위 계산
    Vec2 vResolution = CCore::GetInst()->GetResolution();
    Vec2 vCameraPos = CCamera::GetInst()->GetLookAt();

    Vec2 vScreenStart = vCameraPos - vResolution / 2.f;
    Vec2 vScreenEnd = vCameraPos + vResolution / 2.f;

    _vStart = GetGridPosition(vScreenStart);
    _vEnd = GetGridPosition(vScreenEnd);
}

void CGrid::SetGridSizePreset(int _iPreset)
{
    switch (_iPreset)
    {
    case 1:
        m_fGridSize = 32.f;
        break;
    case 2:
        m_fGridSize = 64.f;
        break;
    case 3:
        m_fGridSize = 128.f;
        break;
    case 4:
        m_fGridSize = 256.f;
        break;
    default:
        m_fGridSize = 64.f;
        break;
    }
}