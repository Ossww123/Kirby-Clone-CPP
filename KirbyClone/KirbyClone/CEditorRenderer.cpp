#include "pch.h"
#include "CEditorRenderer.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"

#include "CCamera.h"
#include "CCore.h"
#include "CGrid.h"
#include "CObjectFactory.h"
#include "CObject.h"

CEditorRenderer::CEditorRenderer()
    : m_pEditorCore(nullptr)
{
}

CEditorRenderer::~CEditorRenderer()
{
}

void CEditorRenderer::Initialize(CEditorCore* _pCore)
{
    m_pEditorCore = _pCore;
}

void CEditorRenderer::Render(HDC _dc)
{
    // 렌더링 순서 (z-order)

    // 1. 플레이어 스폰 포인트 (배경 위에)
    if (m_pEditorCore->GetObjectManager()->IsShowPlayerSpawn())
    {
        RenderPlayerSpawnPoint(_dc);
    }

    // 2. 선택된 오브젝트 하이라이트
    RenderSelectedObject(_dc);

    // 3. 배치 미리보기 (마우스 위에)
    RenderPreview(_dc);

    // 4. 마우스 커서 (맨 위)
    RenderMouse(_dc);
}

void CEditorRenderer::RenderMouse(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_pEditorCore->GetMousePos());
    COLORREF cursorColor = GetModeColor();

    // 마우스 커서 그리기
    RenderMouseCursor(_dc, vRenderPos, cursorColor);

    // 클릭했을 때 원 그리기
    if (m_pEditorCore->IsMouseClick())
    {
        HPEN hClickPen = CreatePen(PS_SOLID, 3, cursorColor);
        HPEN hOldClickPen = (HPEN)SelectObject(_dc, hClickPen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

        Ellipse(_dc,
            (int)vRenderPos.x - 15, (int)vRenderPos.y - 15,
            (int)vRenderPos.x + 15, (int)vRenderPos.y + 15);

        SelectObject(_dc, hOldClickPen);
        SelectObject(_dc, hOldBrush);
        DeleteObject(hClickPen);
    }
}

void CEditorRenderer::RenderPreview(HDC _dc)
{
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode();

    // 배치 모드들에서 미리보기
    if (eMode == EDITOR_MODE::PLACE_MONSTER ||
        eMode == EDITOR_MODE::PLACE_ITEM ||
        eMode == EDITOR_MODE::PLACE_TILE ||
        eMode == EDITOR_MODE::PLACE_SPECIAL)
    {
        Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(m_pEditorCore->GetMousePos());
        OBJECT_TYPE eObjectType = m_pEditorCore->GetObjectManager()->GetCurrentObjectType();
        Vec2 vObjectSize = CObjectFactory::GetDefaultScale(eObjectType);
        COLORREF previewColor = GetPreviewColor();

        RenderPreviewObject(_dc, vRenderPos, vObjectSize, previewColor);
    }
    // 삭제 모드에서 삭제 대상 표시
    else if (eMode == EDITOR_MODE::ERASE)
    {
        CObject* pTargetObj = m_pEditorCore->GetObjectManager()->FindObjectAtPosition(m_pEditorCore->GetMousePos());
        if (pTargetObj)
        {
            RenderDeletePreview(_dc, pTargetObj);
        }
    }
}

void CEditorRenderer::RenderSelectedObject(HDC _dc)
{
    CObject* pSelectedObj = m_pEditorCore->GetSelectedObject();
    if (!pSelectedObj)
        return;

    RenderSelectionBox(_dc, pSelectedObj);

    // 드래그 중이면 이동 경로 표시
    if (m_pEditorCore->IsDragging())
    {
        Vec2 vDragStartRender = CCamera::GetInst()->GetRenderPos(m_pEditorCore->GetDragStartPos());
        Vec2 vCurrentRender = CCamera::GetInst()->GetRenderPos(pSelectedObj->GetPos());

        // 점선으로 이동 경로 표시
        HPEN hDragPen = CreatePen(PS_DOT, 1, RGB(255, 255, 100));
        HPEN hOldDragPen = (HPEN)SelectObject(_dc, hDragPen);

        MoveToEx(_dc, (int)vDragStartRender.x, (int)vDragStartRender.y, nullptr);
        LineTo(_dc, (int)vCurrentRender.x, (int)vCurrentRender.y);

        SelectObject(_dc, hOldDragPen);
        DeleteObject(hDragPen);
    }
}

void CEditorRenderer::RenderPlayerSpawnPoint(HDC _dc)
{
    Vec2 vSpawnPos = m_pEditorCore->GetObjectManager()->GetPlayerSpawnPos();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vSpawnPos);

    // 플레이어 스폰 포인트 표시 (초록색 원과 십자가)
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 외곽 원
    Ellipse(_dc,
        (int)vRenderPos.x - 20, (int)vRenderPos.y - 20,
        (int)vRenderPos.x + 20, (int)vRenderPos.y + 20);

    // 십자가
    DrawCross(_dc, vRenderPos, 15, RGB(0, 255, 0), 3);

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);

    // "SPAWN" 텍스트 표시
    DrawTextWithBackground(_dc,
        Vec2(vRenderPos.x - 15, vRenderPos.y - 35),
        L"SPAWN",
        RGB(0, 255, 0),
        RGB(0, 0, 0));
}

void CEditorRenderer::RenderMouseCursor(HDC _dc, Vec2 vRenderPos, COLORREF color)
{
    // 마우스 커서 그리기 (십자가)
    DrawCross(_dc, vRenderPos, 8, color);

    // 그리드 스냅이 활성화된 경우 그리드 셀 표시
    if (CGrid::GetInst()->IsSnapToGrid())
    {
        RenderGridPreview(_dc, vRenderPos);
    }
}

void CEditorRenderer::RenderPreviewObject(HDC _dc, Vec2 vRenderPos, Vec2 vObjectSize, COLORREF color)
{
    // 반투명 효과를 위한 펜과 브러시 설정
    HPEN hPen = CreatePen(PS_SOLID, 2, color);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = CreateHatchBrush(HS_DIAGCROSS, color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 미리보기 사각형 그리기
    Rectangle(_dc,
        (int)(vRenderPos.x - vObjectSize.x / 2.f),
        (int)(vRenderPos.y - vObjectSize.y / 2.f),
        (int)(vRenderPos.x + vObjectSize.x / 2.f),
        (int)(vRenderPos.y + vObjectSize.y / 2.f));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);

    // 오브젝트 이름 표시
    const wchar_t* szObjectName = m_pEditorCore->GetObjectManager()->GetCurrentObjectName();
    Vec2 vTextPos = Vec2(vRenderPos.x, vRenderPos.y - vObjectSize.y / 2.f - 20);
    DrawTextWithBackground(_dc, vTextPos, szObjectName, color);

    // 타일 모드인 경우 시각 타입도 표시
    if (m_pEditorCore->GetCurrentMode() == EDITOR_MODE::PLACE_TILE)
    {
        const wchar_t* szTileVisual = m_pEditorCore->GetObjectManager()->GetTileVisualName(
            m_pEditorCore->GetObjectManager()->GetCurrentTileVisual());

        Vec2 vTileTextPos = Vec2(vRenderPos.x, vRenderPos.y - vObjectSize.y / 2.f - 35);
        DrawTextWithBackground(_dc, vTileTextPos, szTileVisual, RGB(200, 200, 255));
    }
}

void CEditorRenderer::RenderDeletePreview(HDC _dc, CObject* pTargetObj)
{
    Vec2 vPos = pTargetObj->GetPos();
    Vec2 vScale = pTargetObj->GetScale();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 삭제 대상 표시 (빨간색 X표시)
    int halfSize = (int)(max(vScale.x, vScale.y) / 2.f + 10);
    DrawX(_dc, vRenderPos, halfSize, RGB(255, 100, 100));

    // "DELETE" 텍스트 표시
    Vec2 vTextPos = Vec2(vRenderPos.x - 20, vRenderPos.y - halfSize - 20);
    DrawTextWithBackground(_dc, vTextPos, L"DELETE", RGB(255, 100, 100));
}

void CEditorRenderer::RenderSelectionBox(HDC _dc, CObject* pObj)
{
    Vec2 vPos = pObj->GetPos();
    Vec2 vScale = pObj->GetScale();
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(vPos);

    // 선택 표시 (노란색 테두리)
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 선택 사각형 (약간 더 크게)
    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f - 3),
        (int)(vRenderPos.y - vScale.y / 2.f - 3),
        (int)(vRenderPos.x + vScale.x / 2.f + 3),
        (int)(vRenderPos.y + vScale.y / 2.f + 3));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);
}

void CEditorRenderer::RenderGridPreview(HDC _dc, Vec2 vRenderPos)
{
    COLORREF gridColor = GetModeColor();
    float fGridSize = CGrid::GetInst()->GetGridSize();

    // 그리드 셀 경계 표시
    DrawDottedRectangle(_dc, vRenderPos, Vec2(fGridSize, fGridSize), gridColor);
}

COLORREF CEditorRenderer::GetModeColor()
{
    switch (m_pEditorCore->GetCurrentMode())
    {
    case EDITOR_MODE::PLACE_MONSTER:    return RGB(255, 100, 100); // 빨간색
    case EDITOR_MODE::PLACE_ITEM:       return RGB(255, 255, 100); // 노란색
    case EDITOR_MODE::PLACE_TILE:       return RGB(100, 100, 255); // 파란색
    case EDITOR_MODE::PLACE_SPECIAL:    return RGB(255, 100, 255); // 자주색
    case EDITOR_MODE::SELECT:           return RGB(100, 200, 255); // 하늘색
    case EDITOR_MODE::ERASE:            return RGB(255, 100, 100); // 빨간색
    case EDITOR_MODE::BACKGROUND:       return RGB(100, 255, 100); // 녹색
    case EDITOR_MODE::PLAYER_SPAWN:     return RGB(0, 255, 0);     // 초록색
    case EDITOR_MODE::CAMERA_MOVE:      return RGB(255, 255, 100); // 노란색
    default:                            return RGB(255, 255, 255); // 흰색
    }
}

COLORREF CEditorRenderer::GetPreviewColor()
{
    switch (m_pEditorCore->GetCurrentMode())
    {
    case EDITOR_MODE::PLACE_MONSTER:    return RGB(255, 150, 150); // 연한 빨간색
    case EDITOR_MODE::PLACE_ITEM:       return RGB(255, 255, 150); // 연한 노란색
    case EDITOR_MODE::PLACE_TILE:       return RGB(150, 150, 255); // 연한 파란색
    case EDITOR_MODE::PLACE_SPECIAL:    return RGB(255, 150, 255); // 연한 자주색
    default:                            return RGB(200, 200, 200); // 회색
    }
}

void CEditorRenderer::DrawCross(HDC _dc, Vec2 vPos, int size, COLORREF color, int thickness)
{
    HPEN hPen = CreatePen(PS_SOLID, thickness, color);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 가로선
    MoveToEx(_dc, (int)vPos.x - size, (int)vPos.y, nullptr);
    LineTo(_dc, (int)vPos.x + size, (int)vPos.y);

    // 세로선
    MoveToEx(_dc, (int)vPos.x, (int)vPos.y - size, nullptr);
    LineTo(_dc, (int)vPos.x, (int)vPos.y + size);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CEditorRenderer::DrawX(HDC _dc, Vec2 vPos, int size, COLORREF color, int thickness)
{
    HPEN hPen = CreatePen(PS_SOLID, thickness, color);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    // 대각선 1
    MoveToEx(_dc, (int)vPos.x - size, (int)vPos.y - size, nullptr);
    LineTo(_dc, (int)vPos.x + size, (int)vPos.y + size);

    // 대각선 2
    MoveToEx(_dc, (int)vPos.x + size, (int)vPos.y - size, nullptr);
    LineTo(_dc, (int)vPos.x - size, (int)vPos.y + size);

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CEditorRenderer::DrawDottedRectangle(HDC _dc, Vec2 vPos, Vec2 vSize, COLORREF color)
{
    HPEN hPen = CreatePen(PS_DOT, 1, color);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    Rectangle(_dc,
        (int)(vPos.x - vSize.x / 2.f),
        (int)(vPos.y - vSize.y / 2.f),
        (int)(vPos.x + vSize.x / 2.f),
        (int)(vPos.y + vSize.y / 2.f));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush);
    DeleteObject(hPen);
}

void CEditorRenderer::DrawTextWithBackground(HDC _dc, Vec2 vPos, const wchar_t* text, COLORREF textColor, COLORREF bgColor)
{
    // 텍스트 크기 측정
    SIZE textSize;
    GetTextExtentPoint32(_dc, text, (int)wcslen(text), &textSize);

    // 배경 사각형 그리기
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBgBrush);

    Rectangle(_dc,
        (int)vPos.x - 2,
        (int)vPos.y - 2,
        (int)vPos.x + textSize.cx + 2,
        (int)vPos.y + textSize.cy + 2);

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBgBrush);

    // 텍스트 그리기
    SetTextColor(_dc, textColor);
    SetBkMode(_dc, TRANSPARENT);

    HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(_dc, hFont);

    TextOut(_dc, (int)vPos.x, (int)vPos.y, text, (int)wcslen(text));

    SelectObject(_dc, hOldFont);
    DeleteObject(hFont);
}