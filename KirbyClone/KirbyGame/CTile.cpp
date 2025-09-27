#include "gamePCH.h"
#include "CTile.h"
#include "CAnimator.h"
#include "CCollider.h"
#include "CAnimationDataMgr.h"

// ===== 생성자 =====
CTile::CTile(const Vec2& worldLT, const Vec2& tileSize, TILE_COLLISION type)
    : m_worldLT(worldLT)
    , m_tileSize(tileSize)
    , m_collision(type)
{
    // 오브젝트 위치는 "중심" 기준.
    // 처음엔 visualOffset 없이 셀 중심에 둠.
    const Vec2 center = Vec2(worldLT.x + tileSize.x * 0.5f,
        worldLT.y + tileSize.y * 0.5f);
    SetPos(center);
    SetScale(Vec2(1.f, 1.f));
}

// ===== 생명주기 =====
void CTile::Init()
{
    buildVisual();
    buildCollider();
}

void CTile::Update()
{
    // 타일 자체 로직 없음(애니는 Animator 경로에서 처리)
}

// ===== 비주얼(애니) =====
void CTile::SetVisual(const std::wstring& animFile,
    const std::wstring& animName,
    const Vec2& visualOffset)
{
    m_animFile = animFile;
    m_animName = animName;
    m_visualOffset = visualOffset;

    // 렌더 기준 위치 = 셀 중심 + 시각 오프셋
    const Vec2 cellCenter = Vec2(m_worldLT.x + m_tileSize.x * 0.5f,
        m_worldLT.y + m_tileSize.y * 0.5f);
    SetPos(cellCenter + m_visualOffset);

    buildVisual();
    buildCollider(); // 콜라이더는 -visualOffset 보정이 필요하므로 재구축
}

void CTile::SetCollisionType(TILE_COLLISION type)
{
    if (m_collision == type) return;
    m_collision = type;
    buildCollider();
}

void CTile::SetOneWayThicknessPx(float px)
{
    m_oneWayThicknessPx = (px < 1.f ? 1.f : px);
    if (m_collision == TILE_COLLISION::ONEWAY_TOP)
        buildCollider();
}

// ===== 내부: 비주얼 구성 =====
void CTile::buildVisual()
{
    if (m_animName.empty())
        return;

    if (!GetAnimator())
        CreateAnimator();

    if (!m_animFile.empty())
        CAnimationDataMgr::GetInst()->LoadAnimationsIntoAnimator(GetAnimator(), m_animFile);

    GetAnimator()->Play(m_animName, true);
    // 주의: 엔진에 별도의 RenderOffset API가 없으므로
    //       시각 오프셋은 오브젝트 위치로 반영하고(위 SetVisual),
    //       콜라이더에서 -visualOffset으로 상쇄한다.
}

// ===== 내부: 콜라이더 구성 =====
void CTile::buildCollider()
{
    // 충돌 없음
    if (m_collision == TILE_COLLISION::NONE)
    {
        if (GetCollider())
            GetCollider()->SetScale(Vec2(0.f, 0.f)); // 간단 비활성화
        return;
    }

    if (!GetCollider())
        CreateCollider();

    switch (m_collision)
    {
    case TILE_COLLISION::SOLID:
    {
        // 타일 셀 전체 충돌. 오브젝트가 visualOffset으로 움직였으니,
        // 콜라이더는 그만큼 반대로 보정해서 셀 위치에 정렬.
        GetCollider()->SetOffsetPos(Vec2(-m_visualOffset.x, -m_visualOffset.y));
        GetCollider()->SetScale(m_tileSize);
        break;
    }
    case TILE_COLLISION::ONEWAY_TOP:
    {
        const float h = (std::min)(m_oneWayThicknessPx, m_tileSize.y);
        const float topStripCenterY = -m_tileSize.y * 0.5f + h * 0.5f; // 셀 중심 기준 상단 스트립
        // visualOffset 보정 포함
        GetCollider()->SetOffsetPos(Vec2(-m_visualOffset.x, topStripCenterY - m_visualOffset.y));
        GetCollider()->SetScale(Vec2(m_tileSize.x, h));
        // 실제 ‘위에서만 막힘’ 로직은 CollisionMgr의 그룹 페어에서 처리 예정
        break;
    }
    case TILE_COLLISION::HAZARD:
    {
        GetCollider()->SetOffsetPos(Vec2(-m_visualOffset.x, -m_visualOffset.y));
        GetCollider()->SetScale(m_tileSize);
        // 센서로 쓸 경우 좌표 보정(해결 X) 분기는 상위 충돌 처리에서
        break;
    }
    default: break;
    }
}
