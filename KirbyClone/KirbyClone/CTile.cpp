#include "pch.h"
#include "CTile.h"
#include "CCamera.h"
#include "CCollider.h"
#include "CTexture.h"
#include "CTileMgr.h"
#include <cmath>

CTile::CTile()
    : m_eTileType(OBJECT_TYPE::TILE_GROUND)
    , m_eVisualType(TILE_VISUAL_TYPE::GRASS_PLATFORM)
    , m_pTileTexture(nullptr)
    , m_bSolid(true)
    , m_bHarmful(false)
    , m_bDecorative(false)
{
    // 기본 타일 설정 - 콜라이더는 필요시에만 생성
}

CTile::~CTile()
{
    // 텍스처는 CResMgr에서 관리하므로 여기서 삭제하지 않음
    m_pTileTexture = nullptr;
}

void CTile::Update()
{
    // 타일은 기본적으로 정적이므로 특별한 업데이트 없음
    // TODO: 특수 타일 (움직이는 플랫폼 등) 구현시 여기에 추가

    // 움직이는 플랫폼 예시
    if (m_eVisualType == TILE_VISUAL_TYPE::MOVING_PLATFORM)
    {
        // TODO: 움직이는 플랫폼 로직
    }
}

void CTile::Render(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 1. 전용 텍스처가 있으면 텍스처로 렌더링
    if (m_pTileTexture)
    {
        UINT width = m_pTileTexture->GetWidth();
        UINT height = m_pTileTexture->GetHeight();

        // 마젠타 투명 처리를 위한 TransparentBlt 사용
        TransparentBlt(_dc,
            (int)(vRenderPos.x - vScale.x / 2.f),
            (int)(vRenderPos.y - vScale.y / 2.f),
            (int)vScale.x,
            (int)vScale.y,
            m_pTileTexture->GetDC(),
            0, 0,
            width, height,
            RGB(255, 0, 255)); // 마젠타 투명 처리
    }
    else
    {
        // 2. 텍스처가 없으면 기존 방식으로 색상 렌더링
        HBRUSH hBrush = nullptr;
        COLORREF tileColor = RGB(100, 100, 100); // 기본 회색

        switch (m_eVisualType)
        {
        case TILE_VISUAL_TYPE::GRASS_PLATFORM:
        case TILE_VISUAL_TYPE::GRASS_BLOCK:
            tileColor = RGB(34, 139, 34);   // 초록색 (잔디)
            break;
        case TILE_VISUAL_TYPE::DIRT_BLOCK:
            tileColor = RGB(139, 69, 19);   // 갈색 (흙)
            break;
        case TILE_VISUAL_TYPE::STONE_BLOCK:
            tileColor = RGB(128, 128, 128); // 회색 (돌)
            break;
        case TILE_VISUAL_TYPE::TREE:
            tileColor = RGB(34, 100, 34);   // 진한 초록 (나무)
            break;
        case TILE_VISUAL_TYPE::FLOWER:
            tileColor = RGB(255, 182, 193); // 분홍색 (꽃)
            break;
        case TILE_VISUAL_TYPE::FENCE:
            tileColor = RGB(160, 82, 45);   // 갈색 (나무 울타리)
            break;
        case TILE_VISUAL_TYPE::PIPE:
            tileColor = RGB(0, 128, 0);     // 초록색 (파이프)
            break;
        case TILE_VISUAL_TYPE::SPIKE:
            tileColor = RGB(255, 0, 0);     // 빨간색 (가시)
            break;
        case TILE_VISUAL_TYPE::WATER:
            tileColor = RGB(0, 100, 200);   // 파란색 (물)
            break;
        case TILE_VISUAL_TYPE::LAVA:
            tileColor = RGB(255, 69, 0);    // 주황색 (용암)
            break;
        case TILE_VISUAL_TYPE::MOVING_PLATFORM:
            tileColor = RGB(105, 105, 105); // 어두운 회색 (움직이는 플랫폼)
            break;
        case TILE_VISUAL_TYPE::BRIDGE:
            tileColor = RGB(139, 69, 19);   // 갈색 (다리)
            break;
        default:
            tileColor = RGB(139, 69, 19);   // 기본 갈색
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

        // 3. 특수 타일 추가 표시 (텍스처가 없을 때만)
        RenderSpecialEffects(_dc, vRenderPos, vScale);
    }

    // 4. 콜라이더가 있고 장식용이 아니면 콜라이더도 렌더링 (디버그용)
    if (nullptr != GetCollider() && !m_bDecorative)
    {
        GetCollider()->Render(_dc);
    }
}

void CTile::RenderSpecialEffects(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    switch (m_eVisualType)
    {
    case TILE_VISUAL_TYPE::SPIKE:
        RenderSpikes(_dc, vRenderPos, vScale);
        break;
    case TILE_VISUAL_TYPE::WATER:
        RenderWaterEffect(_dc, vRenderPos, vScale);
        break;
    case TILE_VISUAL_TYPE::TREE:
        RenderTreeDetails(_dc, vRenderPos, vScale);
        break;
    case TILE_VISUAL_TYPE::FLOWER:
        RenderFlowerDetails(_dc, vRenderPos, vScale);
        break;
    }
}

void CTile::RenderSpikes(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    // 가시 모양 그리기 (간단한 삼각형들)
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    int spikeCount = max(1, (int)(vScale.x / 16.f)); // 16픽셀마다 가시 하나, 최소 1개
    float spikeWidth = vScale.x / spikeCount;

    for (int i = 0; i < spikeCount; ++i)
    {
        float startX = vRenderPos.x - vScale.x / 2.f + i * spikeWidth;
        float endX = startX + spikeWidth;
        float midX = startX + spikeWidth / 2.f;
        float topY = vRenderPos.y - vScale.y / 2.f;
        float bottomY = vRenderPos.y + vScale.y / 2.f;

        // 삼각형 가시 그리기
        MoveToEx(_dc, (int)startX, (int)bottomY, nullptr);
        LineTo(_dc, (int)midX, (int)topY);
        LineTo(_dc, (int)endX, (int)bottomY);
    }

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CTile::RenderWaterEffect(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    // 물결 효과 (간단한 웨이브 라인)
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);

    float waveY = vRenderPos.y - vScale.y / 4.f;
    float startX = vRenderPos.x - vScale.x / 2.f;
    float endX = vRenderPos.x + vScale.x / 2.f;

    // 간단한 웨이브 라인
    MoveToEx(_dc, (int)startX, (int)waveY, nullptr);
    for (float x = startX; x < endX; x += 8.f)
    {
        float y = waveY + sin(x * 0.1f) * 4.f;
        LineTo(_dc, (int)x, (int)y);
    }

    SelectObject(_dc, hOldPen);
    DeleteObject(hPen);
}

void CTile::RenderTreeDetails(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    // 나무 줄기 (갈색 직사각형)
    HBRUSH hTrunkBrush = CreateSolidBrush(RGB(101, 67, 33));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hTrunkBrush);

    float trunkWidth = vScale.x * 0.3f;
    float trunkHeight = vScale.y * 0.4f;

    Rectangle(_dc,
        (int)(vRenderPos.x - trunkWidth / 2.f),
        (int)(vRenderPos.y + vScale.y / 2.f - trunkHeight),
        (int)(vRenderPos.x + trunkWidth / 2.f),
        (int)(vRenderPos.y + vScale.y / 2.f));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hTrunkBrush);

    // 나무 잎 (초록색 원)
    HBRUSH hLeafBrush = CreateSolidBrush(RGB(34, 139, 34));
    hOldBrush = (HBRUSH)SelectObject(_dc, hLeafBrush);

    float leafRadius = vScale.x * 0.4f;

    Ellipse(_dc,
        (int)(vRenderPos.x - leafRadius),
        (int)(vRenderPos.y - vScale.y / 2.f),
        (int)(vRenderPos.x + leafRadius),
        (int)(vRenderPos.y - vScale.y / 2.f + leafRadius * 2));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hLeafBrush);
}

void CTile::RenderFlowerDetails(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    // 꽃잎 (분홍색 원들)
    HBRUSH hPetalBrush = CreateSolidBrush(RGB(255, 182, 193));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hPetalBrush);

    float petalSize = vScale.x * 0.3f;

    // 4개 방향으로 꽃잎
    Vec2 petalOffsets[] = {
        Vec2(0.f, -petalSize * 0.5f),  // 위
        Vec2(petalSize * 0.5f, 0.f),   // 오른쪽
        Vec2(0.f, petalSize * 0.5f),   // 아래
        Vec2(-petalSize * 0.5f, 0.f)   // 왼쪽
    };

    for (int i = 0; i < 4; ++i)
    {
        Vec2 petalPos = vRenderPos + petalOffsets[i];
        Ellipse(_dc,
            (int)(petalPos.x - petalSize / 4.f),
            (int)(petalPos.y - petalSize / 4.f),
            (int)(petalPos.x + petalSize / 4.f),
            (int)(petalPos.y + petalSize / 4.f));
    }

    SelectObject(_dc, hOldBrush);
    DeleteObject(hPetalBrush);

    // 꽃 중심 (노란색)
    HBRUSH hCenterBrush = CreateSolidBrush(RGB(255, 255, 0));
    hOldBrush = (HBRUSH)SelectObject(_dc, hCenterBrush);

    float centerSize = vScale.x * 0.15f;
    Ellipse(_dc,
        (int)(vRenderPos.x - centerSize),
        (int)(vRenderPos.y - centerSize),
        (int)(vRenderPos.x + centerSize),
        (int)(vRenderPos.y + centerSize));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hCenterBrush);
}

void CTile::SetupTileByVisualType(TILE_VISUAL_TYPE _eVisualType)
{
    // CTileMgr을 통해 자동 설정
    CTileMgr::GetInst()->SetupTileProperties(this, _eVisualType);
}

void CTile::OnCollisionEnter(CCollider* _pOther)
{
    // 장식용 타일은 충돌 처리하지 않음
    if (m_bDecorative)
        return;

    CObject* pOtherObj = _pOther->GetOwner();

    // 플레이어와 충돌시 데미지 처리
    if (m_bHarmful)
    {
        // TODO: 플레이어에게 데미지 전달
        // 가시, 용암 등에서 데미지 처리

        // 예시: 플레이어 클래스가 있다면
        // CPlayer* pPlayer = dynamic_cast<CPlayer*>(pOtherObj);
        // if (pPlayer) 
        // {
        //     pPlayer->TakeDamage(1);
        // }
    }
}