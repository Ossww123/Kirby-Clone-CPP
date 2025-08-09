#include "pch.h"
#include "CTile.h"
#include "CCamera.h"
#include "CCollider.h"
#include "CCore.h"
#include "CSceneMgr.h"
#include "CScene_Tool.h"

CTile::CTile()
    : m_eTileType(OBJECT_TYPE::TILE_GROUND)
    , m_eCollisionType(COLLISION_TYPE::SOLID_GROUND)
    , m_bSolid(true)
    , m_bHarmful(false)
    , m_bOneWay(false)
    , m_bDecorative(false)
    , m_displayColor(RGB(0, 0, 255))     // 기본: 파란색 (단단한 땅)
    , m_bShowInEditor(true)
    , m_eVisualType(TILE_VISUAL_TYPE::GRASS_PLATFORM)  // 호환성용
    , m_pTileTexture(nullptr)            // 더 이상 사용 안함
{
    // 기본 충돌체 타입 설정
    SetCollisionType(COLLISION_TYPE::SOLID_GROUND);
}

CTile::~CTile()
{
    // 텍스처는 사용하지 않으므로 정리할 것 없음
    m_pTileTexture = nullptr;
}

void CTile::Update()
{
    // 충돌체 전용이므로 특별한 업데이트 로직 없음
    // 필요시 움직이는 플랫폼 등의 로직을 여기에 추가 가능

    if (m_eCollisionType == COLLISION_TYPE::MOVING_PLATFORM)
    {
        // TODO: 움직이는 플랫폼 로직
    }
}

void CTile::Render(HDC _dc)
{
    if (!m_bShowInEditor)
        return;

    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();
    float fPixelScale = CCore::GetPixelScale();
    Vec2 vScaledSize = vScale * fPixelScale;

    // 충돌체 타입에 따른 색상 박스 렌더링 (4배 확대)
    HBRUSH hBrush = CreateSolidBrush(m_displayColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 반투명한 효과를 위해 테두리와 내부를 다르게 렌더링
    Rectangle(_dc,
        (int)(vRenderPos.x - vScaledSize.x / 2.f),
        (int)(vRenderPos.y - vScaledSize.y / 2.f),
        (int)(vRenderPos.x + vScaledSize.x / 2.f),
        (int)(vRenderPos.y + vScaledSize.y / 2.f));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 충돌체가 있으면 충돌체도 스케일 렌더링
    if (GetCollider())
    {
        GetCollider()->RenderScaled(_dc, fPixelScale);
    }
}

void CTile::SetCollisionType(COLLISION_TYPE _eType)
{
    m_eCollisionType = _eType;
    SetupCollisionDefaults(_eType);
    UpdateCollisionProperties();
}

void CTile::SetupCollisionDefaults(COLLISION_TYPE _eType)
{
    // 충돌체 타입별 기본 속성 및 색깔 설정
    switch (_eType)
    {
    case COLLISION_TYPE::SOLID_GROUND:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(0, 0, 255);        // 파란색
        break;

    case COLLISION_TYPE::PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = true;
        m_displayColor = RGB(0, 255, 0);        // 초록색
        break;

    case COLLISION_TYPE::SPIKE:
        m_bSolid = true;
        m_bHarmful = true;
        m_bOneWay = false;
        m_displayColor = RGB(255, 0, 0);        // 빨간색
        break;

    case COLLISION_TYPE::WATER:
        m_bSolid = false;                       // 통과 가능
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(100, 200, 255);    // 연파란색
        break;

    case COLLISION_TYPE::LAVA:
        m_bSolid = false;                       // 통과 가능하지만 데미지
        m_bHarmful = true;
        m_bOneWay = false;
        m_displayColor = RGB(255, 100, 0);      // 주황색
        break;

    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = true;
        m_displayColor = RGB(100, 255, 100);    // 연초록색
        break;

    case COLLISION_TYPE::MOVING_PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(255, 0, 255);      // 보라색
        break;

    case COLLISION_TYPE::BREAKABLE_BLOCK:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(139, 69, 19);      // 황토색
        break;

    case COLLISION_TYPE::INVISIBLE_WALL:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(128, 128, 128);    // 회색
        break;

    default:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB(0, 0, 255);        // 기본 파란색
        break;
    }
}

void CTile::UpdateCollisionProperties()
{
    // 충돌체가 있으면 속성에 따라 업데이트
    if (GetCollider())
    {
        // 충돌체 크기는 64x64로 고정
        GetCollider()->SetScale(Vec2(64.f, 64.f));

        // 단단하지 않은 충돌체는 콜라이더를 트리거 모드로 설정할 수 있음
        // (필요시 CCollider에 트리거 모드 추가)
    }
    else if (m_bSolid || m_bHarmful)
    {
        // 충돌이 필요한 타입이면 콜라이더 생성
        CreateCollider();
        if (GetCollider())
        {
            GetCollider()->SetScale(Vec2(64.f, 64.f));
        }
    }
}

void CTile::RenderCollisionBox(HDC _dc)
{
    Vec2 vRenderPos = CCamera::GetInst()->GetRenderPos(GetPos());
    Vec2 vScale = GetScale();

    // 충돌체 타입에 따른 색깔 박스 렌더링
    HBRUSH hBrush = CreateSolidBrush(m_displayColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(_dc, hBrush);

    // 약간 투명한 효과를 위해 테두리와 내부를 다르게 렌더링
    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f),
        (int)(vRenderPos.y - vScale.y / 2.f),
        (int)(vRenderPos.x + vScale.x / 2.f),
        (int)(vRenderPos.y + vScale.y / 2.f));

    SelectObject(_dc, hOldBrush);
    DeleteObject(hBrush);

    // 테두리 그리기 (더 진한 색으로)
    COLORREF borderColor = RGB(
        GetRValue(m_displayColor) / 2,
        GetGValue(m_displayColor) / 2,
        GetBValue(m_displayColor) / 2
    );

    HPEN hPen = CreatePen(PS_SOLID, 2, borderColor);
    HPEN hOldPen = (HPEN)SelectObject(_dc, hPen);
    HBRUSH hHollowBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(_dc, hHollowBrush);

    Rectangle(_dc,
        (int)(vRenderPos.x - vScale.x / 2.f),
        (int)(vRenderPos.y - vScale.y / 2.f),
        (int)(vRenderPos.x + vScale.x / 2.f),
        (int)(vRenderPos.y + vScale.y / 2.f));

    SelectObject(_dc, hOldPen);
    SelectObject(_dc, hOldBrush2);
    DeleteObject(hPen);

    // 특수 표시 추가
    RenderSpecialIndicators(_dc, vRenderPos, vScale);
}

void CTile::RenderSpecialIndicators(HDC _dc, Vec2 vRenderPos, Vec2 vScale)
{
    // 충돌체 타입별 특수 표시
    SetBkMode(_dc, TRANSPARENT);
    SetTextColor(_dc, RGB(255, 255, 255));

    wchar_t szIndicator[8] = L"";

    switch (m_eCollisionType)
    {
    case COLLISION_TYPE::SPIKE:
        wcscpy_s(szIndicator, L"wan");        // 경고 표시
        break;
    case COLLISION_TYPE::WATER:
        wcscpy_s(szIndicator, L"~~~");         // 물결 표시
        break;
    case COLLISION_TYPE::LAVA:
        wcscpy_s(szIndicator, L"fire");       // 불 표시 (유니코드 지원시)
        break;
    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        wcscpy_s(szIndicator, L"UP");        // 위쪽 화살표
        break;
    case COLLISION_TYPE::MOVING_PLATFORM:
        wcscpy_s(szIndicator, L"LEFRIG");        // 좌우 화살표
        break;
    case COLLISION_TYPE::BREAKABLE_BLOCK:
        wcscpy_s(szIndicator, L"BOOM");       // 폭발 표시 (유니코드 지원시)
        break;
    }

    if (wcslen(szIndicator) > 0)
    {
        TextOut(_dc,
            (int)(vRenderPos.x - 8),
            (int)(vRenderPos.y - 8),
            szIndicator,
            (int)wcslen(szIndicator));
    }
}

void CTile::RenderCollisionInfo(HDC _dc)
{
    // 선택사항: 충돌체 정보를 텍스트로 표시
    // 이 함수는 필요시 구현 (에디터에서 상세 정보 표시용)
}