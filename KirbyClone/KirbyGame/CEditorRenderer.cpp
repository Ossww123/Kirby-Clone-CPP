#include "pch.h"
#include "CEditorRenderer.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"

#include "CKeyMgr.h"
#include "CCamera.h"
#include "CCore.h"
#include "CGrid.h"
#include "CObjectFactory.h"
#include "CObject.h"
#include "CMonster.h"

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

void CEditorRenderer::Render ( HDC _dc )
{
    // 렌더링 순서 (z-order)

    // 0. 게임 경계선 (모든 오브젝트 아래)
    RenderGameOverLine ( _dc );     // 게임 오버선
    RenderLeftBoundaryLine ( _dc ); // 왼쪽 경계선

    // 1. 플레이어 스폰 포인트 (가장 뒤쪽)
    if ( m_pEditorCore->GetObjectManager ( )->IsShowPlayerSpawn ( ) )
    {
        RenderPlayerSpawnPoint ( _dc );
    }

    // 2. 선택된 오브젝트 하이라이트
    RenderSelectedObject ( _dc );

    // 3. 배치 미리보기 (마우스 커서)
    RenderPreview ( _dc );

    // 4. 마우스 커서 (맨 위)
    RenderMouse ( _dc );
}

void CEditorRenderer::RenderMouse ( HDC _dc )
{
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( m_pEditorCore->GetMousePos ( ) );
    COLORREF cursorColor = GetModeColor ( );

    // 마우스 커서 그리기
    RenderMouseCursor ( _dc , vRenderPos , cursorColor );

    // 클릭했을 때 링 그리기
    if ( KEY_HOLD ( KEY::MOUSE_LEFT ) )
    {
        HPEN hClickPen = CreatePen ( PS_SOLID , 3 , cursorColor );
        HPEN hOldClickPen = ( HPEN ) SelectObject ( _dc , hClickPen );
        HBRUSH hBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

        Ellipse ( _dc ,
            ( int ) vRenderPos.x - 15 , ( int ) vRenderPos.y - 15 ,
            ( int ) vRenderPos.x + 15 , ( int ) vRenderPos.y + 15 );

        SelectObject ( _dc , hOldClickPen );
        SelectObject ( _dc , hOldBrush );
        DeleteObject ( hClickPen );
    }
}

void CEditorRenderer::RenderPreview ( HDC _dc )
{
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode ( );

    // 배치 모드에서 미리보기
    if ( eMode == EDITOR_MODE::PLACE_MONSTER ||
        eMode == EDITOR_MODE::PLACE_ITEM ||
        eMode == EDITOR_MODE::PLACE_TILE ||
        eMode == EDITOR_MODE::PLACE_SPECIAL )
    {
        Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( m_pEditorCore->GetMousePos ( ) );
        OBJECT_TYPE eObjectType = m_pEditorCore->GetObjectManager ( )->GetCurrentObjectType ( );
        Vec2 vObjectSize = CObjectFactory::GetDefaultScale ( eObjectType );
        COLORREF previewColor = GetPreviewColor ( );

        RenderPreviewObject ( _dc , vRenderPos , vObjectSize , previewColor );
    }
    // 삭제 모드에서 삭제 대상 표시
    else if ( eMode == EDITOR_MODE::ERASE )
    {
        CObject* pTargetObj = m_pEditorCore->GetObjectManager ( )->FindObjectAtPos ( m_pEditorCore->GetMousePos ( ) );
        if ( pTargetObj )
        {
            RenderDeletePreview ( _dc , pTargetObj );
        }
    }
}

void CEditorRenderer::RenderSelectedObject ( HDC _dc )
{
    CObject* pSelectedObj = m_pEditorCore->GetSelectedObject ( );
    if ( !pSelectedObj )
        return;

    RenderSelectionBox ( _dc , pSelectedObj );

    // 몬스터인 경우 방향 화살표 표시
    if ( pSelectedObj->GetType ( ) >= OBJECT_TYPE::MONSTER_WADDLE_DEE &&
        pSelectedObj->GetType ( ) <= OBJECT_TYPE::MONSTER_WHISPY_WOODS )
    {
        CMonster* pMonster = dynamic_cast< CMonster* >( pSelectedObj );
        if ( pMonster )
        {
            RenderMonsterDirectionArrow ( _dc , pMonster );
        }
    }

    // 드래그 중이면 이동 경로 표시
    if ( m_pEditorCore->IsDragging ( ) )
    {
        Vec2 vDragStartRender = CCamera::GetInst ( )->GetRenderPos ( m_pEditorCore->GetDragStartPos ( ) );
        Vec2 vCurrentRender = CCamera::GetInst ( )->GetRenderPos ( pSelectedObj->GetPos ( ) );

        // 점선으로 이동 경로 표시
        HPEN hDragPen = CreatePen ( PS_DOT , 1 , RGB ( 255 , 255 , 100 ) );
        HPEN hOldDragPen = ( HPEN ) SelectObject ( _dc , hDragPen );

        MoveToEx ( _dc , ( int ) vDragStartRender.x , ( int ) vDragStartRender.y , nullptr );
        LineTo ( _dc , ( int ) vCurrentRender.x , ( int ) vCurrentRender.y );

        SelectObject ( _dc , hOldDragPen );
        DeleteObject ( hDragPen );
    }
}

void CEditorRenderer::RenderPlayerSpawnPoint ( HDC _dc )
{
    Vec2 vSpawnPos = m_pEditorCore->GetObjectManager ( )->GetPlayerSpawnPos ( );
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( vSpawnPos );

    // 플레이어 스폰 포인트 표시 (초록색 원과 십자가)
    HPEN hPen = CreatePen ( PS_SOLID , 3 , RGB ( 0 , 255 , 0 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 원형 테두리
    Ellipse ( _dc ,
        ( int ) vRenderPos.x - 20 , ( int ) vRenderPos.y - 20 ,
        ( int ) vRenderPos.x + 20 , ( int ) vRenderPos.y + 20 );

    // 십자가
    DrawCross ( _dc , vRenderPos , 15 , RGB ( 0 , 255 , 0 ) , 3 );

    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hPen );

    // "SPAWN" 텍스트 표시
    DrawTextWithBackground ( _dc ,
        Vec2 ( vRenderPos.x - 15 , vRenderPos.y - 35 ) ,
        L"SPAWN" ,
        RGB ( 0 , 255 , 0 ) ,
        RGB ( 0 , 0 , 0 ) );
}

void CEditorRenderer::RenderMouseCursor ( HDC _dc , Vec2 vRenderPos , COLORREF color )
{
    // 마우스 커서 그리기 (십자가)
    DrawCross ( _dc , vRenderPos , 8 , color );

    // 그리드 스냅이 활성화된 경우 그리드 셀 표시
    if ( CGrid::GetInst ( )->IsSnapToGrid ( ) )
    {
        RenderGridPreview ( _dc , vRenderPos );
    }
}

void CEditorRenderer::RenderPreviewObject ( HDC _dc , Vec2 vRenderPos , Vec2 vObjectSize , COLORREF color )
{
    // 반투명 효과를 위한 해치 브러시 사용
    HPEN hPen = CreatePen ( PS_SOLID , 2 , color );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hBrush = CreateHatchBrush ( HS_DIAGCROSS , color );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 미리보기 사각형 그리기
    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vObjectSize.x / 2.f ) ,
        ( int ) ( vRenderPos.y - vObjectSize.y / 2.f ) ,
        ( int ) ( vRenderPos.x + vObjectSize.x / 2.f ) ,
        ( int ) ( vRenderPos.y + vObjectSize.y / 2.f ) );

    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hPen );
    DeleteObject ( hBrush );

    // 오브젝트 이름 표시
    const wchar_t* szObjectName = m_pEditorCore->GetObjectManager ( )->GetCurrentObjectName ( );
    Vec2 vTextPos = Vec2 ( vRenderPos.x , vRenderPos.y - vObjectSize.y / 2.f - 20 );
    DrawTextWithBackground ( _dc , vTextPos , szObjectName , color );

    // 타일 모드일 때 비주얼 타입도 표시
    if ( m_pEditorCore->GetCurrentMode ( ) == EDITOR_MODE::PLACE_TILE )
    {
        const wchar_t* szTileVisual = m_pEditorCore->GetObjectManager ( )->GetTileVisualName (
            m_pEditorCore->GetObjectManager ( )->GetCurrentTileVisual ( ) );

        Vec2 vTileTextPos = Vec2 ( vRenderPos.x , vRenderPos.y - vObjectSize.y / 2.f - 35 );
        DrawTextWithBackground ( _dc , vTileTextPos , szTileVisual , RGB ( 200 , 200 , 255 ) );
    }
}

void CEditorRenderer::RenderDeletePreview ( HDC _dc , CObject* pTargetObj )
{
    Vec2 vPos = pTargetObj->GetPos ( );
    Vec2 vScale = pTargetObj->GetScale ( );
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( vPos );

    // 삭제 대상 표시 (빨간색 X표시)
    int halfSize = ( int ) ( max ( vScale.x , vScale.y ) / 2.f + 10 );
    DrawX ( _dc , vRenderPos , halfSize , RGB ( 255 , 100 , 100 ) );

    // "DELETE" 텍스트 표시
    Vec2 vTextPos = Vec2 ( vRenderPos.x - 20 , vRenderPos.y - halfSize - 20 );
    DrawTextWithBackground ( _dc , vTextPos , L"DELETE" , RGB ( 255 , 100 , 100 ) );
}

void CEditorRenderer::RenderSelectionBox ( HDC _dc , CObject* pObj )
{
    Vec2 vPos = pObj->GetPos ( );
    Vec2 vScale = pObj->GetScale ( );
    Vec2 vRenderPos = CCamera::GetInst ( )->GetRenderPos ( vPos );

    // 선택 표시 (노란색 테두리)
    HPEN hPen = CreatePen ( PS_SOLID , 3 , RGB ( 255 , 255 , 0 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    // 선택 사각형 (약간 큰 크기)
    Rectangle ( _dc ,
        ( int ) ( vRenderPos.x - vScale.x / 2.f - 3 ) ,
        ( int ) ( vRenderPos.y - vScale.y / 2.f - 3 ) ,
        ( int ) ( vRenderPos.x + vScale.x / 2.f + 3 ) ,
        ( int ) ( vRenderPos.y + vScale.y / 2.f + 3 ) );

    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hPen );
}

void CEditorRenderer::RenderGridPreview ( HDC _dc , Vec2 vRenderPos )
{
    COLORREF gridColor = GetModeColor ( );
    float fGridSize = CGrid::GetInst ( )->GetGridSize ( );

    // 그리드 셀 미리 표시
    DrawDottedRectangle ( _dc , vRenderPos , Vec2 ( fGridSize , fGridSize ) , gridColor );
}

void CEditorRenderer::RenderGameOverLine ( HDC _dc )
{
    // 맵의 크기 가져오기
    Vec2 vMapSize = m_pEditorCore->GetMapSize ( );
    const float TILE_SIZE = 64.f;  // 기본 타일 크기
    const float DEATH_LINE_Y = vMapSize.y + ( TILE_SIZE * 1.5f );  // 맵 하단에서 1.5타일 아래

    // 화면 전체 너비에 게임오버선 그리기
    Vec2 vResolution = CCore::GetInst ( )->GetResolution ( );
    Vec2 vCameraPos = CCamera::GetInst ( )->GetLookAt ( );

    // 화면 좌우 경계 구하기
    float fScreenLeft = vCameraPos.x - vResolution.x / 2.f;
    float fScreenRight = vCameraPos.x + vResolution.x / 2.f;

    // 월드 좌표를 화면 좌표로 변환
    Vec2 vLeftPoint = CCamera::GetInst ( )->GetRenderPos ( Vec2 ( fScreenLeft , DEATH_LINE_Y ) );
    Vec2 vRightPoint = CCamera::GetInst ( )->GetRenderPos ( Vec2 ( fScreenRight , DEATH_LINE_Y ) );

    // 게임오버선이 화면에 보이는지 체크
    if ( DEATH_LINE_Y >= vCameraPos.y - vResolution.y / 2.f &&
        DEATH_LINE_Y <= vCameraPos.y + vResolution.y / 2.f )
    {
        // 빨간 굵은선으로 게임오버선 그리기
        HPEN hWarningPen = CreatePen ( PS_SOLID , 3 , RGB ( 255 , 0 , 0 ) );
        HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hWarningPen );

        MoveToEx ( _dc , ( int ) vLeftPoint.x , ( int ) vLeftPoint.y , nullptr );
        LineTo ( _dc , ( int ) vRightPoint.x , ( int ) vRightPoint.y );

        // "DEATH LINE" 텍스트 표시
        SetTextColor ( _dc , RGB ( 255 , 0 , 0 ) );
        SetBkMode ( _dc , TRANSPARENT );

        HFONT hFont = CreateFont ( 16 , 0 , 0 , 0 , FW_BOLD , FALSE , FALSE , FALSE ,
            DEFAULT_CHARSET , OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
            DEFAULT_QUALITY , DEFAULT_PITCH | FF_SWISS , L"Arial" );
        HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

        wchar_t szWarning[ ] = L"GAMEOVER LINE (1.5 tiles below map)";
        TextOut ( _dc , ( int ) vLeftPoint.x + 10 , ( int ) vLeftPoint.y - 25 , szWarning , ( int ) wcslen ( szWarning ) );

        SelectObject ( _dc , hOldFont );
        SelectObject ( _dc , hOldPen );
        DeleteObject ( hFont );
        DeleteObject ( hWarningPen );
    }
}

void CEditorRenderer::RenderLeftBoundaryLine ( HDC _dc )
{
    const float LEFT_BOUNDARY_X = 0.f;

    // 화면 전체 높이에 왼쪽 경계선 그리기
    Vec2 vResolution = CCore::GetInst ( )->GetResolution ( );
    Vec2 vCameraPos = CCamera::GetInst ( )->GetLookAt ( );

    // 화면 상하 경계 구하기
    float fScreenTop = vCameraPos.y - vResolution.y / 2.f;
    float fScreenBottom = vCameraPos.y + vResolution.y / 2.f;

    // 월드 좌표를 화면 좌표로 변환
    Vec2 vTopPoint = CCamera::GetInst ( )->GetRenderPos ( Vec2 ( LEFT_BOUNDARY_X , fScreenTop ) );
    Vec2 vBottomPoint = CCamera::GetInst ( )->GetRenderPos ( Vec2 ( LEFT_BOUNDARY_X , fScreenBottom ) );

    // 경계선이 화면에 보이는지 체크
    if ( LEFT_BOUNDARY_X >= vCameraPos.x - vResolution.x / 2.f &&
        LEFT_BOUNDARY_X <= vCameraPos.x + vResolution.x / 2.f )
    {
        // 빨간 점선으로 경계선 그리기
        HPEN hBoundaryPen = CreatePen ( PS_DOT , 2 , RGB ( 255 , 100 , 100 ) );
        HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hBoundaryPen );

        MoveToEx ( _dc , ( int ) vTopPoint.x , ( int ) vTopPoint.y , nullptr );
        LineTo ( _dc , ( int ) vBottomPoint.x , ( int ) vBottomPoint.y );

        // "LEFT BOUNDARY" 텍스트 표시 (세로로)
        SetTextColor ( _dc , RGB ( 255 , 100 , 100 ) );
        SetBkMode ( _dc , TRANSPARENT );

        HFONT hFont = CreateFont ( 14 , 0 , 900 , 0 , FW_BOLD , FALSE , FALSE , FALSE ,  // 900 = 90도 회전
            DEFAULT_CHARSET , OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
            DEFAULT_QUALITY , DEFAULT_PITCH | FF_SWISS , L"Arial" );
        HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

        wchar_t szBoundary[ ] = L"LEFT BOUNDARY (X: 0)";
        TextOut ( _dc , ( int ) vTopPoint.x + 5 , ( int ) vTopPoint.y + 50 , szBoundary , ( int ) wcslen ( szBoundary ) );

        SelectObject ( _dc , hOldFont );
        SelectObject ( _dc , hOldPen );
        DeleteObject ( hFont );
        DeleteObject ( hBoundaryPen );
    }
}

COLORREF CEditorRenderer::GetModeColor ( )
{
    switch ( m_pEditorCore->GetCurrentMode ( ) )
    {
    case EDITOR_MODE::PLACE_MONSTER:    return RGB ( 255 , 100 , 100 ); // 빨간색
    case EDITOR_MODE::PLACE_ITEM:       return RGB ( 255 , 255 , 100 ); // 노란색
    case EDITOR_MODE::PLACE_TILE:       return RGB ( 100 , 100 , 255 ); // 파란색
    case EDITOR_MODE::PLACE_SPECIAL:    return RGB ( 255 , 100 , 255 ); // 자주색
    case EDITOR_MODE::PLACE_STAGE:      return RGB ( 100 , 255 , 255 ); // 청록색
    case EDITOR_MODE::SELECT:           return RGB ( 100 , 200 , 255 ); // 하늘색
    case EDITOR_MODE::ERASE:            return RGB ( 255 , 100 , 100 ); // 빨간색
    case EDITOR_MODE::BACKGROUND:       return RGB ( 100 , 255 , 100 ); // 녹색
    case EDITOR_MODE::PLAYER_SPAWN:     return RGB ( 0 , 255 , 0 );     // 초록색
    default:                            return RGB ( 255 , 255 , 255 ); // 흰색
    }
}

COLORREF CEditorRenderer::GetPreviewColor ( )
{
    switch ( m_pEditorCore->GetCurrentMode ( ) )
    {
    case EDITOR_MODE::PLACE_MONSTER:    return RGB ( 255 , 150 , 150 ); // 연한 빨간색
    case EDITOR_MODE::PLACE_ITEM:       return RGB ( 255 , 255 , 150 ); // 연한 노란색
    case EDITOR_MODE::PLACE_TILE:       return RGB ( 150 , 150 , 255 ); // 연한 파란색
    case EDITOR_MODE::PLACE_SPECIAL:    return RGB ( 255 , 150 , 255 ); // 연한 자주색
    case EDITOR_MODE::PLACE_STAGE:      return RGB ( 150 , 255 , 255 ); // 연한 청록색
    default:                            return RGB ( 200 , 200 , 200 ); // 회색
    }
}

void CEditorRenderer::DrawCross ( HDC _dc , Vec2 vPos , int size , COLORREF color , int thickness )
{
    HPEN hPen = CreatePen ( PS_SOLID , thickness , color );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );

    // 가로선
    MoveToEx ( _dc , ( int ) vPos.x - size , ( int ) vPos.y , nullptr );
    LineTo ( _dc , ( int ) vPos.x + size , ( int ) vPos.y );

    // 세로선
    MoveToEx ( _dc , ( int ) vPos.x , ( int ) vPos.y - size , nullptr );
    LineTo ( _dc , ( int ) vPos.x , ( int ) vPos.y + size );

    SelectObject ( _dc , hOldPen );
    DeleteObject ( hPen );
}

void CEditorRenderer::DrawX ( HDC _dc , Vec2 vPos , int size , COLORREF color , int thickness )
{
    HPEN hPen = CreatePen ( PS_SOLID , thickness , color );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );

    // 대각선 1
    MoveToEx ( _dc , ( int ) vPos.x - size , ( int ) vPos.y - size , nullptr );
    LineTo ( _dc , ( int ) vPos.x + size , ( int ) vPos.y + size );

    // 대각선 2
    MoveToEx ( _dc , ( int ) vPos.x + size , ( int ) vPos.y - size , nullptr );
    LineTo ( _dc , ( int ) vPos.x - size , ( int ) vPos.y + size );

    SelectObject ( _dc , hOldPen );
    DeleteObject ( hPen );
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

void CEditorRenderer::DrawTextWithBackground ( HDC _dc , Vec2 vPos , const wchar_t* text , COLORREF textColor , COLORREF bgColor )
{
    // 텍스트 크기 계산
    SIZE textSize;
    GetTextExtentPoint32 ( _dc , text , ( int ) wcslen ( text ) , &textSize );

    // 배경 사각형 그리기
    HBRUSH hBgBrush = CreateSolidBrush ( bgColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBgBrush );

    Rectangle ( _dc ,
        ( int ) vPos.x - 2 ,
        ( int ) vPos.y - 2 ,
        ( int ) vPos.x + textSize.cx + 2 ,
        ( int ) vPos.y + textSize.cy + 2 );

    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hBgBrush );

    // 텍스트 그리기
    SetTextColor ( _dc , textColor );
    SetBkMode ( _dc , TRANSPARENT );

    HFONT hFont = CreateFont ( 12 , 0 , 0 , 0 , FW_NORMAL , FALSE , FALSE , FALSE ,
        DEFAULT_CHARSET , OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
        DEFAULT_QUALITY , DEFAULT_PITCH | FF_DONTCARE , L"Arial" );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

    TextOut ( _dc , ( int ) vPos.x , ( int ) vPos.y , text , ( int ) wcslen ( text ) );

    SelectObject ( _dc , hOldFont );
    DeleteObject ( hFont );
}

void CEditorRenderer::RenderMonsterDirectionArrow(HDC _dc, CMonster* _pMonster)
{
    Vec2 vMonsterPos = CCamera::GetInst()->GetRenderPos(_pMonster->GetPos());
    int direction = _pMonster->GetDirection();
    
    // 화살표 설정
    int arrowSize = 20;
    int arrowOffset = 40; // 몬스터로부터 얼마나 떨어뜨릴지
    
    HPEN hArrowPen = CreatePen(PS_SOLID, 3, RGB(255, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(_dc, hArrowPen);
    
    if (direction < 0) // 왼쪽 방향
    {
        // 왼쪽 화살표 그리기
        int arrowX = (int)vMonsterPos.x - arrowOffset;
        int arrowY = (int)vMonsterPos.y;
        
        // 화살표 본체 (가로선)
        MoveToEx(_dc, arrowX - arrowSize, arrowY, nullptr);
        LineTo(_dc, arrowX, arrowY);
        
        // 화살표 머리 (위쪽)
        MoveToEx(_dc, arrowX, arrowY, nullptr);
        LineTo(_dc, arrowX + arrowSize/3, arrowY - arrowSize/2);
        
        // 화살표 머리 (아래쪽)
        MoveToEx(_dc, arrowX, arrowY, nullptr);
        LineTo(_dc, arrowX + arrowSize/3, arrowY + arrowSize/2);
    }
    else if (direction > 0) // 오른쪽 방향
    {
        // 오른쪽 화살표 그리기
        int arrowX = (int)vMonsterPos.x + arrowOffset;
        int arrowY = (int)vMonsterPos.y;
        
        // 화살표 본체 (가로선)
        MoveToEx(_dc, arrowX - arrowSize, arrowY, nullptr);
        LineTo(_dc, arrowX, arrowY);
        
        // 화살표 머리 (위쪽)
        MoveToEx(_dc, arrowX, arrowY, nullptr);
        LineTo(_dc, arrowX - arrowSize/3, arrowY - arrowSize/2);
        
        // 화살표 머리 (아래쪽)
        MoveToEx(_dc, arrowX, arrowY, nullptr);
        LineTo(_dc, arrowX - arrowSize/3, arrowY + arrowSize/2);
    }
    
    SelectObject(_dc, hOldPen);
    DeleteObject(hArrowPen);
}