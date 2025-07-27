#include "pch.h"
#include "CTile.h"
#include "CCamera.h"
#include "CCollider.h"

CTile::CTile()
    : m_eTileType(OBJECT_TYPE::TILE_GROUND)
    , m_bSolid(true)
    , m_bHarmful(false)
{
    // 기본 타일 설정

    // 고체 타일에만 충돌체 생성
    if (m_bSolid)
    {
        CreateCollider();
        GetCollider()->SetScale(Vec2(64.f, 64.f)); // 타일 크기와 동일
    }
}

CTile::~CTile()
{
}

void CTile::Update()
{
    // 타일은 기본적으로 움직이지 않음
    // TODO: 특수 타일 (움직이는 플랫폼 등) 구현
}

void CTile::Render(HDC _dc)
{
    // 타일 타입에 따른 색상으로 렌더링
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    HBRUSH hBrush = nullptr;
    COLORREF tileColor = RGB(100, 100, 100); // 기본 회색

    switch (m_eTileType)
    {
    case OBJECT_TYPE::TILE_GROUND:
        tileColor = RGB(139, 69, 19);   // 갈색 (땅)
        break;
    case OBJECT_TYPE::TILE_SPIKE:
        tileColor = RGB(255, 0, 0);     // 빨간색 (가시)
        break;
    case OBJECT_TYPE::TILE_WATER:
        tileColor = RGB(0, 0, 255);     // 파란색 (물)
        break;
    case OBJECT_TYPE::TILE_WARP_STAR:
        tileColor = RGB(255, 255, 0);   // 노란색 (워프스타)
        break;
    }

    hBrush = CreateSolidBrush(tileColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 타일 사각형 그리기
    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f),
        (int)(vRenderPos.y - vScale.y / 2.f),
        (int)(vRenderPos.x + vScale.x / 2.f),
        (int)(vRenderPos.y + vScale.y / 2.f));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 가시 타일인 경우 추가 표시
    if (m_eTileType == OBJECT_TYPE::TILE_SPIKE)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

        // 가시 모양 그리기 (간단한 삼각형들)
        int spikeSize = (int)(vScale.x / 8.f);
        int startX = (int)(vRenderPos.x - vScale.x / 2.f);
        int endX = (int)(vRenderPos.x + vScale.x / 2.f);
        int topY = (int)(vRenderPos.y - vScale.y / 2.f);
        int bottomY = (int)(vRenderPos.y + vScale.y / 2.f);

        for (int x = startX; x < endX; x += spikeSize)
        {
            MoveToEx(_dc, x, bottomY, nullptr);
            LineTo(_dc, x + spikeSize / 2, topY);
            LineTo(_dc, x + spikeSize, bottomY);
        }

        SelectObject(_dc, hOldPen);
        DeleteObject(hPen);
    }

    // 충돌체가 있으면 충돌체도 렌더링 (디버그용)
    if (nullptr != GetCollider())
        GetCollider()->Render(_dc);
}

void CTile::OnCollisionEnter(CCollider* _pOther)
{
    // TODO: 플레이어와 충돌 시 효과 처리 (데미지, 워프 등)
    if (m_bHarmful)
    {
        // 가시 타일 등에서 데미지 처리
        // TODO: 플레이어에게 데미지 전달
    }
}