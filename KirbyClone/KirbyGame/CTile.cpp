#include "gamePCH.h"
#include "CTile.h"
#include "CCamera.h"
#include "CCollider.h"
#include "CCore.h"
#include "CSceneMgr.h"

CTile::CTile ( )
    : m_eTileType ( OBJECT_TYPE::TILE_GROUND )
    , m_eCollisionType ( COLLISION_TYPE::SOLID_GROUND )
    , m_bSolid ( true )
    , m_bHarmful ( false )
    , m_bOneWay ( false )
    , m_displayColor ( RGB ( 0 , 0 , 255 ) )
    , m_eVisualType ( TILE_VISUAL_TYPE::TRANSPARENT_BLOCK )
    , m_pTileTexture ( nullptr )
    , m_vBossLockPos ( Vec2 ( 0.f , 0.f ) )
    , m_bTriggerActive ( true )
{
    // 기본 충돌체 타입 설정
    SetCollisionType ( COLLISION_TYPE::SOLID_GROUND );
}

CTile::~CTile ( )
{
    // 텍스처는 매니저가 관리하므로 삭제할 필요 없음
    m_pTileTexture = nullptr;
}

void CTile::Update ( )
{
    // 충돌체 전용이므로 특별한 업데이트 로직 없음
    // 필요한 경우에는 플랫폼 이동 로직을 여기에 추가 가능

    if ( m_eCollisionType == COLLISION_TYPE::MOVING_PLATFORM )
    {
        // TODO: 움직이는 플랫폼 구현
    }
}

void CTile::Render ( HDC _dc )
{
    // 디버그 시각 요소가 켜져 있을 때만 투명 타일과 트리거 박스 렌더링
    if ( CCore::IsDebugVisualsVisible ( ) )
    {
        // 타일 타입별 렌더링 처리
        switch ( m_eVisualType )
        {
        case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK:
            RenderTransparentBlock ( _dc );
            break;
        case TILE_VISUAL_TYPE::BOSS_TRIGGER:
            RenderTriggerTile ( _dc );
            break;
            // 향후 추가될 타일들
            // case TILE_VISUAL_TYPE::GRASS_PLATFORM:
            //     RenderGrassPlatform(_dc);
            //     break;
        default:
            // 알 수 없는 타일 타입은 기본 색상 블록으로 렌더링
            RenderTransparentBlock ( _dc );
            break;
        }

        // 공통 렌더링 요소들
        RenderCommonElements ( _dc );
    }
}

void CTile::SetCollisionType ( COLLISION_TYPE _eType )
{
    // 충돌 타입 설정
    m_eCollisionType = _eType;

    // 기본 속성 업데이트
    SetupCollisionDefaults ( _eType );
    UpdateCollisionProperties ( );
}

void CTile::SetupCollisionDefaults ( COLLISION_TYPE _eType )
{
    // 충돌체 타입별 기본 속성 및 색상 설정
    switch ( _eType )
    {
    case COLLISION_TYPE::SOLID_GROUND:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 0 , 0 , 255 );        // 파란색
        break;

    case COLLISION_TYPE::PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = true;
        m_displayColor = RGB ( 0 , 255 , 0 );        // 초록색
        break;

    case COLLISION_TYPE::SPIKE:
        m_bSolid = true;
        m_bHarmful = true;
        m_bOneWay = false;
        m_displayColor = RGB ( 255 , 0 , 0 );        // 빨간색
        break;

    case COLLISION_TYPE::WATER:
        m_bSolid = false;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 100 , 200 , 255 );    // 연파란색
        break;

    case COLLISION_TYPE::LAVA:
        m_bSolid = false;
        m_bHarmful = true;
        m_bOneWay = false;
        m_displayColor = RGB ( 255 , 100 , 0 );      // 주황색
        break;

    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = true;
        m_displayColor = RGB ( 100 , 255 , 100 );    // 연초록색
        break;

    case COLLISION_TYPE::MOVING_PLATFORM:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 255 , 0 , 255 );      // 마젠타색
        break;

    case COLLISION_TYPE::BREAKABLE_BLOCK:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 139 , 69 , 19 );      // 갈색
        break;

    case COLLISION_TYPE::INVISIBLE_WALL:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 128 , 128 , 128 );    // 회색
        break;

    case COLLISION_TYPE::TRIGGER:
        m_bSolid = false;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 255 , 0 , 255 );      // 마젠타색
        break;

    default:
        m_bSolid = true;
        m_bHarmful = false;
        m_bOneWay = false;
        m_displayColor = RGB ( 0 , 0 , 255 );        // 기본 파란색
        break;
    }
}

void CTile::UpdateCollisionProperties ( )
{
    // 충돌체 생성/제거 처리
    if ( m_bSolid || m_bHarmful )
    {
        // 충돌이 필요한 타일이면 콜라이더 생성
        if ( !GetCollider ( ) )
        {
            CreateCollider ( );
        }
        if ( GetCollider ( ) )
        {
            GetCollider ( )->SetScale ( GetScale ( ) );
        }
    }
    else if ( IsDecorative ( ) )
    {
        // 장식용 타일이면 콜라이더 제거
        if ( GetCollider ( ) )
        {
            // TODO: 콜라이더 제거 로직 구현 필요
        }
    }
}

void CTile::RenderCollisionBox ( HDC _dc )
{
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( GetPos ( ) );
    Vec2 vScale = GetScale ( );

    // 충돌체 타입에 따른 색상 박스 렌더링
    HBRUSH hBrush = CreateSolidBrush ( m_displayColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 약간 투명한 효과를 위해 테두리와 내부를 다르게 렌더링
    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vScale.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vScale.y / 2.f ) );

    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hBrush );

    // 테두리 그리기 (더 진한 색상으로)
    COLORREF borderColor = RGB (
        GetRValue ( m_displayColor ) / 2 ,
        GetGValue ( m_displayColor ) / 2 ,
        GetBValue ( m_displayColor ) / 2
    );

    HPEN hPen = CreatePen ( PS_SOLID , 2 , borderColor );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    HBRUSH hOldBrush2 = ( HBRUSH ) SelectObject ( _dc , hHollowBrush );

    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vScale.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vScale.y / 2.f ) );

    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush2 );
    DeleteObject ( hPen );

    // 특수 표시 추가
    RenderSpecialIndicators ( _dc , vRenderPos , vScale );
}

void CTile::RenderSpecialIndicators ( HDC _dc , Vec2 vRenderPos , Vec2 vScale )
{
    // 충돌체 타입별 특수 표시
    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    wchar_t szIndicator[ 8 ] = L"";

    switch ( m_eCollisionType )
    {
    case COLLISION_TYPE::SPIKE:
        wcscpy_s ( szIndicator , L"spike" );        // 가시 표시
        break;
    case COLLISION_TYPE::WATER:
        wcscpy_s ( szIndicator , L"~~~" );         // 물결 표시
        break;
    case COLLISION_TYPE::LAVA:
        wcscpy_s ( szIndicator , L"lava" );       // 불 표시 (유니코드 문제로)
        break;
    case COLLISION_TYPE::ONE_WAY_PLATFORM:
        wcscpy_s ( szIndicator , L"UP" );        // 위쪽 화살표
        break;
    case COLLISION_TYPE::MOVING_PLATFORM:
        wcscpy_s ( szIndicator , L"L R" );        // 좌우 화살표
        break;
    case COLLISION_TYPE::BREAKABLE_BLOCK:
        wcscpy_s ( szIndicator , L"destroy" );       // 깨짐 표시 (유니코드 문제로)
        break;
    }

    if ( wcslen ( szIndicator ) > 0 )
    {
        TextOut ( _dc ,
            ( int ) ( vRenderPos.x - 8 ) ,
            ( int ) ( vRenderPos.y - 8 ) ,
            szIndicator ,
            ( int ) wcslen ( szIndicator ) );
    }
}

void CTile::RenderCollisionInfo ( HDC _dc )
{
    // 타일의 상세 텍스트 렌더링
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( GetPos ( ) );

    // 충돌 타입 정보 표시
    wstring strInfo = L"";
    switch ( m_eCollisionType )
    {
    case COLLISION_TYPE::SOLID_GROUND: strInfo = L"SOLID"; break;
    case COLLISION_TYPE::PLATFORM: strInfo = L"PLATFORM"; break;
    case COLLISION_TYPE::SPIKE: strInfo = L"SPIKE"; break;
    default: strInfo = L"UNKNOWN"; break;
    }

    // 텍스트 출력
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
    SetBkMode ( _dc , TRANSPARENT );
    TextOut ( _dc ,
        ( int ) ( vRenderPos.x - 20 ) ,
        ( int ) ( vRenderPos.y - 30 ) ,
        strInfo.c_str ( ) ,
        ( int ) strInfo.length ( ) );
}

// === 타일 타입별 렌더링 ===

void CTile::RenderTransparentBlock ( HDC _dc )
{
    // 렌더링 좌표 계산
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( GetPos ( ) );
    Vec2 vScale = GetScale ( );
    float fPixelScale = CCore::GetPixelScale ( );
    Vec2 vScaledSize = vScale * fPixelScale;
    Vec2 vActualSize = vScale;

    // 충돌체 타입에 따른 색상 박스 렌더링
    HBRUSH hBrush = CreateSolidBrush ( m_displayColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 투명하게 효과를 위해 테두리와 내부를 다르게 렌더링
    /*Rectangle(_dc,
        (int)(vRenderPos.x - vScaledSize.x / 2.f),
        (int)(vRenderPos.y - vScaledSize.y / 2.f),
        (int)(vRenderPos.x + vScaledSize.x / 2.f),
        (int)(vRenderPos.y + vScaledSize.y / 2.f));*/

    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vActualSize.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vActualSize.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vActualSize.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vActualSize.y / 2.f ) );

    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hBrush );
}

void CTile::RenderCommonElements ( HDC _dc )
{
    // 콜라이더 렌더링
    if ( GetCollider ( ) )
    {
        float fPixelScale = CCore::GetPixelScale ( );
        GetCollider ( )->RenderScaled ( _dc , fPixelScale );
    }
}

void CTile::RenderTriggerTile ( HDC _dc )
{
    // 트리거 타일의 독특한 시각 표시 (기본 블록과 좌표 계산)
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( GetPos ( ) );
    Vec2 vScale = GetScale ( );

    // 트리거 타일은 밝은 노란 박스 렌더링
    HBRUSH hBrush = CreateSolidBrush ( RGB ( 255 , 255 , 0 ) );  // 노란색
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 반투명하게 효과를 위해 50% 투명도로
    HBRUSH hPattern = ( HBRUSH ) GetStockObject ( LTGRAY_BRUSH );
    LOGBRUSH logBrush;
    GetObject ( hPattern , sizeof ( LOGBRUSH ) , &logBrush );

    SetBkMode ( _dc , TRANSPARENT );
    SetBkColor ( _dc , RGB ( 255 , 255 , 0 ) );

    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vScale.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vScale.y / 2.f ) );

    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hBrush );

    // 테두리 그리기 (오렌지 색상)
    HPEN hPen = CreatePen ( PS_SOLID , 3 , RGB ( 255 , 165 , 0 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    HBRUSH hOldBrush2 = ( HBRUSH ) SelectObject ( _dc , hHollowBrush );

    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vScale.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vScale.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vScale.y / 2.f ) );

    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush2 );
    DeleteObject ( hPen );

    // 보스전 텍스트 표시
    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 100 , 0 ) );

    //wchar_t szText[ ] = L"BOSS";
    //TextOut ( _dc ,
    //    ( int ) ( vRenderPos.x - 16 ) ,
    //    ( int ) ( vRenderPos.y - 8 ) ,
    //    szText ,
    //    ( int ) wcslen ( szText ) );
}

void CTile::SetVisualType ( TILE_VISUAL_TYPE _eType )
{
    m_eVisualType = _eType;

    // 비주얼 타입에 따라 자동으로 적절한 충돌 타입 설정
    switch ( _eType )
    {
    case TILE_VISUAL_TYPE::TRANSPARENT_BLOCK:
        // TRANSPARENT_BLOCK은 기본적으로 SOLID_GROUND 사용 (기존 동작 유지)
        // 사용자가 에디터에서 다른 충돌 타입을 직접 설정할 수 있음
        break;

    case TILE_VISUAL_TYPE::BOSS_TRIGGER:
        // BOSS_TRIGGER는 자동으로 TRIGGER 충돌 타입 설정
        SetCollisionType ( COLLISION_TYPE::TRIGGER );
        break;

    default:
        // 향후 추가될 다른 비주얼 타입들의 기본 처리
        break;
    }
}