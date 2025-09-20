#include "gamePCH.h"
#include "CEditorUI.h"
#include "CEditorCore.h"
#include "CEditorObjectManager.h"
#include "CEditorToolbar.h"
#include "CEditorCameraController.h"

#include "CCore.h"
#include "CScene.h"
#include "CGrid.h"
#include "CBackgroundMgr.h"
#include "CTexture.h"
#include "CStageImage.h"
#include "CStageMgr.h"
#include "CObject.h"
#include "CDoor.h"
#include "CMonster.h"
#include "CTile.h"
#include "CResMgr.h"

CEditorUI::CEditorUI ( )
    : m_pEditorCore ( nullptr )
    , m_pScene ( nullptr )
{
}

CEditorUI::~CEditorUI ( )
{
}

void CEditorUI::Initialize ( CEditorCore* _pCore , CScene* _pScene )
{
    m_pEditorCore = _pCore;
    m_pScene = _pScene;

    // 화면 해상도 가져오기
    RECT screenRect;
    GetClientRect ( CCore::GetInst ( )->GetMainHwnd ( ) , &screenRect );
    int screenWidth = screenRect.right - screenRect.left;

    // 오브젝트 팔레트 레이아웃 설정 - 화면 오른쪽에 고정된 사이즈
    m_iPaletteWidth = 280;           // 패널 너비
    m_iPaletteHeight = 600;          // 패널 높이
    m_iPaletteX = screenWidth - m_iPaletteWidth - 10;  // 화면 오른쪽에서 10px 여백을 두고 배치
    m_iPaletteY = 60;                // 상단 아래
    m_iItemSize = 60;                // 아이템 크기
    m_iItemPadding = 8;              // 아이템 간격
    m_iItemsPerRow = 4;              // 한 줄에 4개씩
    m_iScrollOffset = 0;

    // 속성 패널 레이아웃 설정 - 팔레트 바로 아래
    m_iPropertyPanelX = m_iPaletteX;                           // 팔레트와 같은 X
    m_iPropertyPanelY = m_iPaletteY + m_iPaletteHeight + 10;   // 팔레트 아래 10px 간격
    m_iPropertyPanelWidth = m_iPaletteWidth;                   // 팔레트와 같은 너비
    m_iPropertyPanelHeight = 250;

    CalculatePaletteLayout ( );
}

void CEditorUI::Render ( HDC _dc )
{
    if ( !m_pEditorCore->IsShowUI ( ) )
        return;

    RenderObjectPalette ( _dc );
    RenderPropertyPanel ( _dc );
}

void CEditorUI::SetupTextStyle ( HDC _dc , COLORREF color )
{
    SetTextColor ( _dc , color );
    SetBkMode ( _dc , TRANSPARENT );
}

void CEditorUI::RenderText ( HDC _dc , int x , int y , const wchar_t* text , COLORREF color )
{
    SetTextColor ( _dc , color );
    TextOut ( _dc , x , y , text , ( int ) wcslen ( text ) );
}

void CEditorUI::RenderBoldText ( HDC _dc , int x , int y , const wchar_t* text , COLORREF color )
{
    HFONT hFont = CreateUIFont ( 14 , true );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

    SetTextColor ( _dc , color );
    TextOut ( _dc , x , y , text , ( int ) wcslen ( text ) );

    SelectObject ( _dc , hOldFont );
    DeleteObject ( hFont );
}

int CEditorUI::GetPaletteItemAt ( Vec2 vMousePos ) const
{
    int startY = m_iPaletteY + 50;
    int relativeX = ( int ) vMousePos.x - ( m_iPaletteX + 10 );
    int relativeY = ( int ) vMousePos.y - startY + m_iScrollOffset;

    if ( relativeX < 0 || relativeY < 0 )
        return -1;

    int col = relativeX / ( m_iItemSize + m_iItemPadding );
    int row = relativeY / ( m_iItemSize + m_iItemPadding );

    if ( col >= m_iItemsPerRow )
        return -1;

    int itemIndex = row * m_iItemsPerRow + col;
    return itemIndex;
}

bool CEditorUI::IsInPaletteArea ( Vec2 vMousePos ) const
{
    return vMousePos.x >= m_iPaletteX && vMousePos.x <= m_iPaletteX + m_iPaletteWidth &&
        vMousePos.y >= m_iPaletteY && vMousePos.y <= m_iPaletteY + m_iPaletteHeight;
}

void CEditorUI::RenderPropertyPanel ( HDC _dc )
{
    // 배경 그리기
    RenderPropertyPanelBackground ( _dc );

    // 헤더 그리기
    RenderPropertyPanelHeader ( _dc );

    // 선택된 오브젝트에 따른 속성 패널들
    CObject* pSelected = m_pEditorCore->GetSelectedObject ( );
    if ( !pSelected )
    {
        RenderNoSelection ( _dc );
        return;
    }

    // 오브젝트 타입에 따른 속성 패널
    if ( pSelected->GetType ( ) == OBJECT_TYPE::OBJECT_DOOR )
    {
        CDoor* pDoor = dynamic_cast< CDoor* >( pSelected );
        if ( pDoor )
        {
            RenderDoorProperties ( _dc , pDoor );
        }
    }
    // 몬스터 타입들 처리
    else if ( pSelected->GetType ( ) >= OBJECT_TYPE::MONSTER_WADDLE_DEE &&
             pSelected->GetType ( ) <= OBJECT_TYPE::MONSTER_WHISPY_WOODS )
    {
        CMonster* pMonster = dynamic_cast< CMonster* >( pSelected );
        if ( pMonster )
        {
            RenderMonsterProperties ( _dc , pMonster );
        }
    }
    // 타일 타입들 처리
    else if ( pSelected->GetType ( ) == OBJECT_TYPE::TILE_GROUND ||
             pSelected->GetType ( ) == OBJECT_TYPE::TILE_TRIGGER )
    {
        CTile* pTile = dynamic_cast< CTile* >( pSelected );
        if ( pTile )
        {
            RenderTileProperties ( _dc , pTile );
        }
    }
    // 다른 오브젝트 타입들도 여기 추가 예정
}

bool CEditorUI::HandlePropertyPanelClick ( Vec2 vMousePos )
{
    CObject* pSelected = m_pEditorCore->GetSelectedObject ( );
    if ( !pSelected )
        return false;

    // 몬스터 속성 편집
    if ( pSelected->GetType ( ) >= OBJECT_TYPE::MONSTER_WADDLE_DEE &&
        pSelected->GetType ( ) <= OBJECT_TYPE::MONSTER_WHISPY_WOODS )
    {
        CMonster* pMonster = dynamic_cast< CMonster* >( pSelected );
        if ( pMonster )
        {
            return HandleMonsterPropertyEdit ( pMonster , vMousePos );
        }
    }

    // 타일 속성 편집
    if ( pSelected->GetType ( ) == OBJECT_TYPE::TILE_GROUND ||
        pSelected->GetType ( ) == OBJECT_TYPE::TILE_TRIGGER )
    {
        CTile* pTile = dynamic_cast< CTile* >( pSelected );
        if ( pTile )
        {
            return HandleTilePropertyEdit ( pTile , vMousePos );
        }
    }

    return false;
}

bool CEditorUI::IsInPropertyPanelArea ( Vec2 vMousePos ) const
{
    return vMousePos.x >= m_iPropertyPanelX && vMousePos.x <= m_iPropertyPanelX + m_iPropertyPanelWidth &&
           vMousePos.y >= m_iPropertyPanelY && vMousePos.y <= m_iPropertyPanelY + m_iPropertyPanelHeight;
}

void CEditorUI::RenderPropertyPanelBackground ( HDC _dc )
{
    // 배경 (팔레트와 동일한 스타일)
    HBRUSH hBrush = CreateSolidBrush ( RGB ( 40 , 40 , 40 ) );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );

    Rectangle ( _dc ,
        m_iPropertyPanelX ,
        m_iPropertyPanelY ,
        m_iPropertyPanelX + m_iPropertyPanelWidth ,
        m_iPropertyPanelY + m_iPropertyPanelHeight );

    // 테두리
    HPEN hPen = CreatePen ( PS_SOLID , 2 , RGB ( 100 , 100 , 100 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );

    HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    HBRUSH hOldHollowBrush = ( HBRUSH ) SelectObject ( _dc , hHollowBrush );

    Rectangle ( _dc ,
        m_iPropertyPanelX ,
        m_iPropertyPanelY ,
        m_iPropertyPanelX + m_iPropertyPanelWidth ,
        m_iPropertyPanelY + m_iPropertyPanelHeight );

    SelectObject ( _dc , hOldBrush );
    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldHollowBrush );
    DeleteObject ( hBrush );
    DeleteObject ( hPen );
}

void CEditorUI::RenderPropertyPanelHeader ( HDC _dc )
{
    // 헤더 배경
    HBRUSH hHeaderBrush = CreateSolidBrush ( RGB ( 60 , 60 , 60 ) );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hHeaderBrush );

    Rectangle ( _dc ,
        m_iPropertyPanelX ,
        m_iPropertyPanelY ,
        m_iPropertyPanelX + m_iPropertyPanelWidth ,
        m_iPropertyPanelY + 30 );

    // 헤더 텍스트
    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    HFONT hFont = CreateUIFont ( 14 , true );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

    TextOut ( _dc , m_iPropertyPanelX + 10 , m_iPropertyPanelY + 8 , L"Properties" , 10 );

    SelectObject ( _dc , hOldBrush );
    SelectObject ( _dc , hOldFont );
    DeleteObject ( hHeaderBrush );
    DeleteObject ( hFont );
}

void CEditorUI::RenderDoorProperties ( HDC _dc , CDoor* _pDoor )
{
    int currentY = m_iPropertyPanelY + 40;  // 헤더 아래부터 시작
    int leftMargin = m_iPropertyPanelX + 10;
    int lineHeight = 20;

    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    // 오브젝트 타입 표시
    RenderBoldText ( _dc , leftMargin , currentY , L"Door Object" , RGB ( 255 , 255 , 100 ) );
    currentY += lineHeight + 5;

    // 구분선
    HPEN hLinePen = CreatePen ( PS_SOLID , 1 , RGB ( 100 , 100 , 100 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hLinePen );
    MoveToEx ( _dc , leftMargin , currentY , NULL );
    LineTo ( _dc , m_iPropertyPanelX + m_iPropertyPanelWidth - 10 , currentY );
    currentY += 10;

    // 현재 설정 표시
    SetTextColor ( _dc , RGB ( 200 , 200 , 200 ) );
    TextOut ( _dc , leftMargin , currentY , L"Current Settings:" , 17 );
    currentY += lineHeight;

    // 목표 씬 정보
    SCENE_TYPE targetScene = _pDoor->GetTargetScene ( );
    wchar_t szSceneInfo[ 64 ];
    swprintf_s ( szSceneInfo , L"  Scene: %s" , GetSceneName ( targetScene ) );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
    TextOut ( _dc , leftMargin , currentY , szSceneInfo , ( int ) wcslen ( szSceneInfo ) );
    currentY += lineHeight;

    // 목표 위치 정보
    Vec2 targetPos = _pDoor->GetTargetPosition ( );
    wchar_t szPosInfo[ 64 ];
    swprintf_s ( szPosInfo , L"  Position: (%.0f, %.0f)" , targetPos.x , targetPos.y );
    TextOut ( _dc , leftMargin , currentY , szPosInfo , ( int ) wcslen ( szPosInfo ) );
    currentY += lineHeight + 10;

    // 조작 가이드
    SetTextColor ( _dc , RGB ( 200 , 200 , 100 ) );
    TextOut ( _dc , leftMargin , currentY , L"Controls:" , 9 );
    currentY += lineHeight;

    SetTextColor ( _dc , RGB ( 180 , 180 , 180 ) );

    // 씬 변경 안내
    TextOut ( _dc , leftMargin , currentY , L"Scene Change:" , 13 );
    currentY += lineHeight - 5;
    TextOut ( _dc , leftMargin + 10 , currentY , L"[1] Stage 1" , 11 );
    currentY += lineHeight - 5;
    TextOut ( _dc , leftMargin + 10 , currentY , L"[2] Stage 2" , 11 );
    currentY += lineHeight;

    // 위치 조작 안내
    TextOut ( _dc , leftMargin , currentY , L"Position Control:" , 17 );
    currentY += lineHeight - 5;
    TextOut ( _dc , leftMargin + 10 , currentY , L"[Q/W] Left/Right Move" , 20 );
    currentY += lineHeight - 5;
    TextOut ( _dc , leftMargin + 10 , currentY , L"[A/S] Up/Down Move" , 17 );
    currentY += lineHeight - 5;
    TextOut ( _dc , leftMargin + 10 , currentY , L"[R] Reset to Default Position" , 29 );

    SelectObject ( _dc , hOldPen );
    DeleteObject ( hLinePen );
}

void CEditorUI::RenderNoSelection ( HDC _dc )
{
    int centerY = m_iPropertyPanelY + m_iPropertyPanelHeight / 2;
    int centerX = m_iPropertyPanelX + m_iPropertyPanelWidth / 2;

    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 128 , 128 , 128 ) );

    // 텍스트 중앙 정렬을 위한 크기 측정
    const wchar_t* text = L"Select Object";
    SIZE textSize;
    GetTextExtentPoint32 ( _dc , text , ( int ) wcslen ( text ) , &textSize );

    TextOut ( _dc ,
        centerX - textSize.cx / 2 ,
        centerY - textSize.cy / 2 ,
        text ,
        ( int ) wcslen ( text ) );
}

bool CEditorUI::HandleDoorPropertyEdit ( CDoor* _pDoor , Vec2 _vPos )
{
    return false;
}

void CEditorUI::RenderSceneDropdown ( HDC _dc , SCENE_TYPE _currentScene , int _x , int _y , int _width , int _height )
{
}

void CEditorUI::RenderInputField ( HDC _dc , const wchar_t* _label , float _value , int _x , int _y , int _width )
{
}

const wchar_t* CEditorUI::GetSceneName ( SCENE_TYPE _eScene ) const
{
    switch ( _eScene )
    {
    case SCENE_TYPE::STAGE_01: return L"Stage 1";
    case SCENE_TYPE::STAGE_02: return L"Stage 2";
    case SCENE_TYPE::START: return L"Start Screen";
    default: return L"Unknown";
    }
}

void CEditorUI::CalculatePaletteLayout ( )
{
    // 스크롤 최댓값 계산 및 레이아웃 갱신 로직
    const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager ( )->GetCurrentCategory ( );
    int totalRows = ( ( int ) vecCategory.size ( ) + m_iItemsPerRow - 1 ) / m_iItemsPerRow;
    int visibleRows = ( m_iPaletteHeight - 60 ) / ( m_iItemSize + m_iItemPadding );
    m_iMaxScroll = max ( 0 , ( totalRows - visibleRows ) * ( m_iItemSize + m_iItemPadding ) );
}

void CEditorUI::RenderObjectPalette ( HDC _dc )
{
    // 배치 모드가 아니면 팔레트 표시 안함
    EDITOR_MODE eMode = m_pEditorCore->GetCurrentMode ( );
    if ( eMode != EDITOR_MODE::PLACE_MONSTER &&
        eMode != EDITOR_MODE::PLACE_ITEM &&
        eMode != EDITOR_MODE::PLACE_TILE &&
        eMode != EDITOR_MODE::PLACE_SPECIAL )
    {
        return;
    }

    RenderPaletteBackground ( _dc );
    RenderPaletteHeader ( _dc );
    RenderPaletteItems ( _dc );
}

bool CEditorUI::HandlePaletteClick ( Vec2 vMousePos )
{
    if ( !IsInPaletteArea ( vMousePos ) )
        return false;

    EDITOR_MODE currentMode = m_pEditorCore->GetCurrentMode ( );

    // Stage Image 모드일 때는 별도 처리
    if ( currentMode == EDITOR_MODE::PLACE_STAGE )
    {
        return HandleStageImagePaletteClick ( vMousePos );
    }

    // 일반 오브젝트 팔레트 클릭 처리
    int clickedIndex = GetPaletteItemAt ( vMousePos );
    if ( clickedIndex >= 0 )
    {
        m_pEditorCore->GetObjectManager ( )->SetCurrentSubType ( clickedIndex );
        return true;
    }

    return false;
}

bool CEditorUI::HandleStageImagePaletteClick ( Vec2 vMousePos )
{
    // 사용 가능한 스테이지 이미지 타입들 가져오기
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst ( )->GetAvailableStageImageTypes ( );

    if ( availableTypes.empty ( ) )
        return false;

    // 클릭한 아이템 인덱스
    int relativeX = ( int ) vMousePos.x - ( m_iPaletteX + 10 );
    int relativeY = ( int ) vMousePos.y - ( m_iPaletteY + 50 ) + m_iScrollOffset;

    if ( relativeX < 0 || relativeY < 0 )
        return false;

    int col = relativeX / ( m_iItemSize + m_iItemPadding );
    int row = relativeY / ( m_iItemSize + m_iItemPadding );
    int clickedIndex = row * m_iItemsPerRow + col;

    // 유효한 인덱스인지 확인
    if ( clickedIndex >= 0 && clickedIndex < ( int ) availableTypes.size ( ) )
    {
        STAGE_IMAGE_TYPE selectedType = availableTypes[ clickedIndex ];
        CStageMgr::GetInst ( )->SetCurrentStageImage ( selectedType );

        wchar_t szBuffer[ 256 ];
        swprintf_s ( szBuffer , L"Stage Image selected: %s" ,
            CStageMgr::GetInst ( )->GetStageImageName ( selectedType ) );
        SetWindowText ( CCore::GetInst ( )->GetMainHwnd ( ) , szBuffer );

        return true;
    }

    return false;
}

void CEditorUI::RenderPaletteBackground ( HDC _dc )
{
    // 팔레트 배경
    HBRUSH hBrush = CreateSolidBrush ( RGB ( 40 , 40 , 40 ) );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );
    Rectangle ( _dc , m_iPaletteX , m_iPaletteY ,
        m_iPaletteX + m_iPaletteWidth , m_iPaletteY + m_iPaletteHeight );

    // 테두리
    HPEN hPen = CreatePen ( PS_SOLID , 2 , RGB ( 100 , 100 , 100 ) );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    SelectObject ( _dc , hHollowBrush );
    Rectangle ( _dc , m_iPaletteX , m_iPaletteY ,
        m_iPaletteX + m_iPaletteWidth , m_iPaletteY + m_iPaletteHeight );

    SelectObject ( _dc , hOldBrush );
    SelectObject ( _dc , hOldPen );
    DeleteObject ( hBrush );
    DeleteObject ( hPen );
}

void CEditorUI::RenderPaletteHeader ( HDC _dc )
{
    // 헤더 배경
    HBRUSH hHeaderBrush = CreateSolidBrush ( RGB ( 80 , 80 , 80 ) );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hHeaderBrush );
    Rectangle ( _dc , m_iPaletteX , m_iPaletteY , m_iPaletteX + m_iPaletteWidth , m_iPaletteY + 40 );

    // 헤더 텍스트
    HFONT hFont = CreateUIFont ( 16 , true );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
    SetBkMode ( _dc , TRANSPARENT );

    const wchar_t* szTitle;
    EDITOR_MODE currentMode = m_pEditorCore->GetCurrentMode ( );

    switch ( currentMode )
    {
    case EDITOR_MODE::PLACE_MONSTER:
        szTitle = L"Monsters";
        break;
    case EDITOR_MODE::PLACE_ITEM:
        szTitle = L"Items";
        break;
    case EDITOR_MODE::PLACE_TILE:
        szTitle = L"Tiles";
        break;
    case EDITOR_MODE::PLACE_SPECIAL:
        szTitle = L"Special Objects";
        break;
    case EDITOR_MODE::PLACE_STAGE:      // 새로 추가
        szTitle = L"Stage Images";
        break;
    default:
        szTitle = L"Objects";
        break;
    }

    TextOut ( _dc , m_iPaletteX + 10 , m_iPaletteY + 12 , szTitle , ( int ) wcslen ( szTitle ) );

    // Stage Image 모드일 때는 현재 선택된 스테이지 표시
    if ( currentMode == EDITOR_MODE::PLACE_STAGE )
    {
        STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst ( )->GetCurrentStageType ( );
        const wchar_t* currentStageName = CStageMgr::GetInst ( )->GetStageImageName ( currentType );

        wchar_t szInfo[ 256 ];
        swprintf_s ( szInfo , L"Current: %s" , currentStageName );
        TextOut ( _dc , m_iPaletteX + 10 , m_iPaletteY + 28 , szInfo , ( int ) wcslen ( szInfo ) );
    }
    else
    {
        // 다른 모드에서는 현재 선택 표시
        const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager ( )->GetCurrentCategory ( );
        wchar_t szInfo[ 256 ];
        swprintf_s ( szInfo , L"(%d/%d)" ,
            m_pEditorCore->GetObjectManager ( )->GetCurrentSubType ( ) + 1 ,
            ( int ) vecCategory.size ( ) );
        TextOut ( _dc , m_iPaletteX + 200 , m_iPaletteY + 12 , szInfo , ( int ) wcslen ( szInfo ) );
    }

    SelectObject ( _dc , hOldBrush );
    SelectObject ( _dc , hOldFont );
    DeleteObject ( hHeaderBrush );
    DeleteObject ( hFont );
}

void CEditorUI::RenderPaletteItems ( HDC _dc )
{
    // 현재 카테고리의 오브젝트들 가져오기
    const vector<OBJECT_TYPE>& vecCategory = m_pEditorCore->GetObjectManager ( )->GetCurrentCategory ( );
    if ( vecCategory.empty ( ) )
        return;

    // 현재 선택된 서브 타입
    int currentSubType = m_pEditorCore->GetObjectManager ( )->GetCurrentSubType ( );

    // 아이템들을 그리기 시작 위치
    int startX = m_iPaletteX + 10;
    int startY = m_iPaletteY + 50; // 헤더 아래

    // 스크롤 오프셋 적용
    int renderY = startY - m_iScrollOffset;

    for ( size_t i = 0; i < vecCategory.size ( ); ++i )
    {
        // 아이템 위치 계산
        int col = i % m_iItemsPerRow;
        int row = i / m_iItemsPerRow;

        int itemX = startX + col * ( m_iItemSize + m_iItemPadding );
        int itemY = renderY + row * ( m_iItemSize + m_iItemPadding );

        // 화면 범위를 벗어남 (최적화)
        if ( itemY + m_iItemSize < m_iPaletteY + 50 ||
            itemY > m_iPaletteY + m_iPaletteHeight )
        {
            continue;
        }

        // 선택 여부 확인
        bool isSelected = ( i == currentSubType );

        // 아이템 렌더링
        RenderPaletteItem ( _dc , ( int ) i , vecCategory[ i ] , itemX , itemY , isSelected );
    }
}

void CEditorUI::RenderPaletteItem ( HDC _dc , int index , OBJECT_TYPE objType , int x , int y , bool selected )
{
    // 아이템 배경
    COLORREF bgColor = selected ? RGB ( 100 , 150 , 100 ) : RGB ( 60 , 60 , 60 );
    HBRUSH hBrush = CreateSolidBrush ( bgColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );
    Rectangle ( _dc , x , y , x + m_iItemSize , y + m_iItemSize );

    // 테두리
    COLORREF borderColor = selected ? RGB ( 150 , 200 , 150 ) : RGB ( 100 , 100 , 100 );
    HPEN hPen = CreatePen ( PS_SOLID , selected ? 3 : 1 , borderColor );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
    SelectObject ( _dc , hHollowBrush );
    Rectangle ( _dc , x , y , x + m_iItemSize , y + m_iItemSize );

    // 오브젝트 아이콘 렌더링 (크게 만들고 중앙 정렬하되 하단 텍스트 공간 확보)
    int iconSize = m_iItemSize - 8; // 아이콘 크기를 크게 (4배 정도)
    int iconX = x + ( m_iItemSize - iconSize ) / 2 + 20; // 가로 중앙 정렬 + 16
    int iconY = y + ( m_iItemSize - iconSize - 18 ) / 2 + 24; // 세로 중앙 정렬 + 16 (텍스트 공간 18px 확보)
    RenderObjectIcon ( _dc , objType , iconX , iconY , iconSize );

    // 오브젝트 이름 표시 (하단에)
    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
    HFONT hFont = CreateUIFont ( 8 , false );  // 작은 폰트 사용
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

    // 타일/충돌체 타입인지 확인
    bool bIsTileType = ( objType >= OBJECT_TYPE::TILE_GROUND && objType <= OBJECT_TYPE::TILE_TRIGGER );

    wchar_t szDisplayName[ 32 ];
    if ( bIsTileType )
    {
        // 충돌체 타입은 충돌체 이름 표시
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType ( objType );
        const wchar_t* szCollisionName = CObjectFactory::GetCollisionTypeName ( collisionType );

        // 이름이 길면 축약 표시
        if ( wcslen ( szCollisionName ) > 10 )
        {
            wcsncpy_s ( szDisplayName , szCollisionName , 8 );
            szDisplayName[ 8 ] = L'.';
            szDisplayName[ 9 ] = L'.';
            szDisplayName[ 10 ] = L'\0';
        }
        else
        {
            wcscpy_s ( szDisplayName , szCollisionName );
        }
    }
    else
    {
        // 다른 오브젝트는 기본 이름 표시
        const wchar_t* szName = CObjectFactory::GetObjectTypeName ( objType );
        if ( wcslen ( szName ) > 10 )
        {
            wcsncpy_s ( szDisplayName , szName , 8 );
            szDisplayName[ 8 ] = L'.';
            szDisplayName[ 9 ] = L'.';
            szDisplayName[ 10 ] = L'\0';
        }
        else
        {
            wcscpy_s ( szDisplayName , szName );
        }
    }

    // 텍스트를 아이템 하단에 표시 (2줄로 나눠서 표시)
    RECT textRect = { x + 2, y + m_iItemSize - 18, x + m_iItemSize - 2, y + m_iItemSize - 2 };
    DrawTextW ( _dc , szDisplayName , -1 , &textRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS );

    // 충돌체일 때 추가 정보 표시
    if ( bIsTileType && selected )
    {
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType ( objType );

        // 속성 표시 (작은 아이콘들)
        int iconY = y + m_iItemSize - 32;
        int iconX = x + 2;

        // 단단함 표시
        if ( collisionType == COLLISION_TYPE::SOLID_GROUND ||
            collisionType == COLLISION_TYPE::SPIKE ||
            collisionType == COLLISION_TYPE::BREAKABLE_BLOCK ||
            collisionType == COLLISION_TYPE::INVISIBLE_WALL )
        {
            SetTextColor ( _dc , RGB ( 100 , 100 , 255 ) );
            TextOut ( _dc , iconX , iconY , L"Solid" , 5 );  // Solid display
            iconX += 20;
        }

        // 위험함 표시
        if ( collisionType == COLLISION_TYPE::SPIKE ||
            collisionType == COLLISION_TYPE::LAVA )
        {
            SetTextColor ( _dc , RGB ( 255 , 100 , 100 ) );
            TextOut ( _dc , iconX , iconY , L"Danger" , 6 );  // Danger display
            iconX += 20;
        }

        // 일방통행 표시
        if ( collisionType == COLLISION_TYPE::PLATFORM ||
            collisionType == COLLISION_TYPE::ONE_WAY_PLATFORM )
        {
            SetTextColor ( _dc , RGB ( 100 , 255 , 100 ) );
            TextOut ( _dc , iconX , iconY , L"TopOnly" , 7 );  // Top only collision
        }
    }

    SelectObject ( _dc , hOldBrush );
    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldFont );
    DeleteObject ( hBrush );
    DeleteObject ( hPen );
    DeleteObject ( hFont );
}

void CEditorUI::RenderObjectIcon ( HDC _dc , OBJECT_TYPE objType , int x , int y , int size )
{
    // 타일/충돌체 타입인지 확인
    bool bIsTileType = ( objType >= OBJECT_TYPE::TILE_GROUND && objType <= OBJECT_TYPE::TILE_TRIGGER );

    if ( bIsTileType )
    {
        // 타일/충돌체 타입은 색상 박스로 표시
        COLLISION_TYPE collisionType = CObjectFactory::ConvertObjectTypeToCollisionType ( objType );
        COLORREF iconColor = CObjectFactory::GetCollisionTypeColor ( collisionType );

        // 충돌체 박스 스타일로 렌더링
        HBRUSH hIconBrush = CreateSolidBrush ( iconColor );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hIconBrush );

        // 사각형으로 그리기 (타일과 같은 모양)
        Rectangle ( _dc , x + 8 , y + 8 , x + size - 8 , y + size - 8 );

        // 테두리 그리기 (더 어두운 색상)
        COLORREF borderColor = RGB (
            GetRValue ( iconColor ) / 2 ,
            GetGValue ( iconColor ) / 2 ,
            GetBValue ( iconColor ) / 2
        );

        HPEN hBorderPen = CreatePen ( PS_SOLID , 2 , borderColor );
        HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hBorderPen );
        HBRUSH hHollowBrush = ( HBRUSH ) GetStockObject ( HOLLOW_BRUSH );
        HBRUSH hOldBrush2 = ( HBRUSH ) SelectObject ( _dc , hHollowBrush );

        Rectangle ( _dc , x + 8 , y + 8 , x + size - 8 , y + size - 8 );

        // 특수 표시 아이콘 추가
        RenderCollisionIcon ( _dc , collisionType , x + size / 2 , y + size / 2 );

        SelectObject ( _dc , hOldBrush );
        SelectObject ( _dc , hOldPen );
        SelectObject ( _dc , hOldBrush2 );
        DeleteObject ( hIconBrush );
        DeleteObject ( hBorderPen );
    }
    else if ( objType >= OBJECT_TYPE::MONSTER_WADDLE_DEE && objType <= OBJECT_TYPE::MONSTER_WHISPY_WOODS )
    {
        // 몬스터 타입은 실제 스프라이트로 표시
        RenderMonsterSprite ( _dc , objType , x , y , size );
    }
    else
    {
        // 다른 오브젝트는 색상 원형으로 표시
        COLORREF iconColor = RGB ( 200 , 200 , 200 ); // 기본색
        
        switch ( objType )
        {
        case OBJECT_TYPE::ITEM_STAR:
            iconColor = RGB ( 255 , 255 , 100 ); // 노란색
            break;
        case OBJECT_TYPE::ITEM_ENERGY_DRINK:
            iconColor = RGB ( 100 , 255 , 100 ); // 초록색
            break;
        case OBJECT_TYPE::ITEM_1UP:
            iconColor = RGB ( 255 , 100 , 255 ); // 보라색
            break;
        case OBJECT_TYPE::ITEM_ABILITY_STAR:
            iconColor = RGB ( 100 , 100 , 255 ); // 파란색
            break;
        case OBJECT_TYPE::OBJECT_DOOR:
            iconColor = RGB ( 139 , 69 , 19 );   // 갈색
            break;
        case OBJECT_TYPE::OBJECT_SWITCH:
            iconColor = RGB ( 255 , 215 , 0 );   // 금색
            break;
        case OBJECT_TYPE::OBJECT_MIRROR:
            iconColor = RGB ( 192 , 192 , 192 ); // 은색
            break;
        default:
            iconColor = RGB ( 150 , 150 , 150 );
            break;
        }

        HBRUSH hIconBrush = CreateSolidBrush ( iconColor );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hIconBrush );

        // 원형으로 오브젝트 아이콘 표시
        Ellipse ( _dc , x + 8 , y + 8 , x + size - 8 , y + size - 8 );

        SelectObject ( _dc , hOldBrush );
        DeleteObject ( hIconBrush );
    }
}

void CEditorUI::RenderMonsterSprite ( HDC _dc , OBJECT_TYPE objType , int x , int y , int size )
{
    CTexture* pTexture = nullptr;
    int srcX = 0, srcY = 0, srcWidth = 24, srcHeight = 24;  // 기본 크기
    
    switch ( objType )
    {
    case OBJECT_TYPE::MONSTER_WADDLE_DEE:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 8; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_WADDLE_DOO:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 40; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_BRONTO_BURT:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 72; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_GORDOS:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 104; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_HOT_HEAD:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 136; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_SPARKY:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"EnemyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"EnemyTex" , L"texture\\enemy\\enemies.bmp" );
        srcX = 8; srcY = 168; srcWidth = 32; srcHeight = 32;
        break;
        
    case OBJECT_TYPE::MONSTER_WHISPY_WOODS:
        pTexture = CResMgr::GetInst ( )->FindTexture ( L"WhispyTex" );
        if ( !pTexture )
            pTexture = CResMgr::GetInst ( )->LoadTexture ( L"WhispyTex" , L"texture\\boss\\whispy_woods.bmp" );
        srcX = 272; srcY = 24; srcWidth = 120; srcHeight = 120;  // 위스피 우드는 크기가 다름
        break;
        
    default:
        // 기본 몬스터 텍스처나 색상으로 fallback
        HBRUSH hIconBrush = CreateSolidBrush ( RGB ( 255 , 100 , 100 ) );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hIconBrush );
        Ellipse ( _dc , x + 8 , y + 8 , x + size - 8 , y + size - 8 );
        SelectObject ( _dc , hOldBrush );
        DeleteObject ( hIconBrush );
        return;
    }
    
    if ( pTexture )
    {
        // 팔레트 크기에 맞게 스케일링 
        int renderSize = size - 8; // 패딩을 줄여서 크게 표시
        int renderX = x + ( size - renderSize ) / 2; // 중앙 정렬
        int renderY = y + ( size - renderSize ) / 2; // 중앙 정렬
        
        // 위스피 우드는 특별히 처리 (세로로 긴 스프라이트)
        if ( objType == OBJECT_TYPE::MONSTER_WHISPY_WOODS )
        {
            // 세로로 긴 스프라이트를 정사각형에 맞게 조정하되 중앙 정렬 유지
            float aspectRatio = ( float ) srcHeight / ( float ) srcWidth;
            if ( aspectRatio > 1.0f )
            {
                // 세로가 더 길 때 - 너비를 조정하고 가로 중앙 정렬
                int adjustedWidth = ( int ) ( renderSize / aspectRatio );
                renderX = x + ( size - adjustedWidth ) / 2;
                renderSize = adjustedWidth; // 실제 렌더링 너비로 업데이트
            }
            else
            {
                // 가로가 더 길 때 - 높이를 조정하고 세로 중앙 정렬
                int adjustedHeight = ( int ) ( renderSize * aspectRatio );
                renderY = y + ( size - adjustedHeight ) / 2;
            }
        }
        
        // CTexture의 스프라이트 렌더링 함수 사용
        pTexture->RenderSpriteWithColorKey ( _dc , 
                                           Vec2 ( ( float ) renderX , ( float ) renderY ) ,
                                           Vec2 ( ( float ) srcX , ( float ) srcY ) ,
                                           Vec2 ( ( float ) srcWidth , ( float ) srcHeight ) ,
                                           Vec2 ( ( float ) renderSize , ( float ) renderSize ) ,
                                           RGB ( 255 , 0 , 255 ) ); // 마젠타 컬러키
    }
    else
    {
        // 텍스처 로딩 실패 시 기본 색상으로 표시
        HBRUSH hIconBrush = CreateSolidBrush ( RGB ( 255 , 100 , 100 ) );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hIconBrush );
        Ellipse ( _dc , x + 8 , y + 8 , x + size - 8 , y + size - 8 );
        SelectObject ( _dc , hOldBrush );
        DeleteObject ( hIconBrush );
    }
}

HFONT CEditorUI::CreateUIFont ( int size , bool bold )
{
    return CreateFont ( size , 0 , 0 , 0 , bold ? FW_BOLD : FW_NORMAL , FALSE , FALSE , FALSE ,
        DEFAULT_CHARSET , OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
        DEFAULT_QUALITY , DEFAULT_PITCH | FF_DONTCARE , L"Arial" );
}

void CEditorUI::RenderCollisionIcon ( HDC _dc , COLLISION_TYPE collisionType , int centerX , int centerY )
{
    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    HFONT hFont = CreateUIFont ( 12 , true );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );

    wchar_t szIcon[ 16 ] = L"";

    switch ( collisionType )
    {
    case COLLISION_TYPE::SOLID_GROUND:
        wcscpy_s ( szIcon , L"Ground" );  // 단단한 땅
        break;
    case COLLISION_TYPE::TRIGGER:
        wcscpy_s ( szIcon , L"Trigger" );  // 트리거
        break;
        //case COLLISION_TYPE::PLATFORM:
        //    wcscpy_s(szIcon, L"발판");  // 플랫폼
        //    break;
        //case COLLISION_TYPE::SPIKE:
        //    wcscpy_s(szIcon, L"가시");  // 가시 (삼각형)
        //    break;
        //case COLLISION_TYPE::WATER:
        //    wcscpy_s(szIcon, L"물");  // 물
        //    break;
        //case COLLISION_TYPE::LAVA:
        //    wcscpy_s(szIcon, L"용암");  // 용암 (다이아몬드 모양)
        //    break;
        //case COLLISION_TYPE::ONE_WAY_PLATFORM:
        //    wcscpy_s(szIcon, L"위");  // 위쪽 화살표
        //    break;
        //case COLLISION_TYPE::MOVING_PLATFORM:
        //    wcscpy_s(szIcon, L"이동");  // 좌우 화살표
        //    break;
        //case COLLISION_TYPE::BREAKABLE_BLOCK:
        //    wcscpy_s(szIcon, L"깨짐");  // 빈 박스
        //    break;
        //case COLLISION_TYPE::INVISIBLE_WALL:
        //    wcscpy_s(szIcon, L"?");  // 물음표
        //    break;
    default:
        wcscpy_s ( szIcon , L"Default" );  // Default type
        break;
    }

    if ( wcslen ( szIcon ) > 0 )
    {
        // 텍스트 크기 측정
        SIZE textSize;
        GetTextExtentPoint32 ( _dc , szIcon , ( int ) wcslen ( szIcon ) , &textSize );

        // 중앙에 배치
        TextOut ( _dc ,
            centerX - textSize.cx / 2 ,
            centerY - textSize.cy / 2 ,
            szIcon ,
            ( int ) wcslen ( szIcon ) );
    }

    SelectObject ( _dc , hOldFont );
    DeleteObject ( hFont );
}

void CEditorUI::RenderStageImagePalette ( HDC _dc )
{
    // 사용 가능한 스테이지 이미지 타입들 가져오기
    vector<STAGE_IMAGE_TYPE> availableTypes = CStageMgr::GetInst ( )->GetAvailableStageImageTypes ( );
    STAGE_IMAGE_TYPE currentType = CStageMgr::GetInst ( )->GetCurrentStageType ( );

    // 팔레트 배경
    RenderPaletteBackground ( _dc );
    RenderPaletteHeader ( _dc );

    // 스테이지 이미지 아이템들 렌더링
    int startX = m_iPaletteX + 10;
    int startY = m_iPaletteY + 50;
    int renderY = startY - m_iScrollOffset;

    for ( size_t i = 0; i < availableTypes.size ( ); ++i )
    {
        STAGE_IMAGE_TYPE stageType = availableTypes[ i ];
        bool isSelected = ( stageType == currentType );

        // 아이템 위치 계산
        int col = i % m_iItemsPerRow;
        int row = i / m_iItemsPerRow;

        int itemX = startX + col * ( m_iItemSize + m_iItemPadding );
        int itemY = renderY + row * ( m_iItemSize + m_iItemPadding );

        // 화면 범위를 벗어남
        if ( itemY + m_iItemSize < m_iPaletteY + 50 ||
            itemY > m_iPaletteY + m_iPaletteHeight )
        {
            continue;
        }

        // 스테이지 이미지 아이템 렌더링
        RenderStageImageItem ( _dc , stageType , itemX , itemY , isSelected );
    }
}

void CEditorUI::RenderStageImageItem ( HDC _dc , STAGE_IMAGE_TYPE stageType , int x , int y , bool selected )
{
    // 아이템 배경
    COLORREF bgColor = selected ? RGB ( 100 , 150 , 100 ) : RGB ( 60 , 60 , 60 );
    HBRUSH hBrush = CreateSolidBrush ( bgColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );
    Rectangle ( _dc , x , y , x + m_iItemSize , y + m_iItemSize );

    // 테두리
    COLORREF borderColor = selected ? RGB ( 150 , 200 , 150 ) : RGB ( 100 , 100 , 100 );
    HPEN hPen = CreatePen ( PS_SOLID , selected ? 2 : 1 , borderColor );
    HPEN hOldPen = ( HPEN ) SelectObject ( _dc , hPen );
    Rectangle ( _dc , x , y , x + m_iItemSize , y + m_iItemSize );

    // 스테이지 타입에 따른 아이콘/이미지표시
    int centerX = x + m_iItemSize / 2;
    int centerY = y + m_iItemSize / 2;

    // 실제 스테이지 이미지가 로드되어 있으면 썸네일 표시, 없으면 아이콘
    CStageImage* pStageImage = CStageMgr::GetInst ( )->FindStageImage ( stageType );
    if ( pStageImage && pStageImage->GetStageTexture ( ) )
    {
        // 미니 썸네일 렌더링 (축소된 버전)
        CTexture* pTexture = pStageImage->GetStageTexture ( );

        int thumbnailSize = m_iItemSize - 10;
        int thumbnailX = x + 5;
        int thumbnailY = y + 5;

        // 텍스처를 축소해서 렌더링
        pTexture->RenderWithColorKey ( _dc ,
            Vec2 ( ( float ) thumbnailX , ( float ) thumbnailY ) ,
            Vec2 ( ( float ) thumbnailSize , ( float ) thumbnailSize ) ,
            RGB ( 255 , 0 , 255 ) ); // 마젠타 컬러키
    }
    else
    {
        // 텍스처가 없으면 타입별 아이콘 표시
        RenderStageImageIcon ( _dc , stageType , centerX , centerY );
    }

    // 스테이지 이름 표시 (아이템 하단)
    const wchar_t* stageName = CStageMgr::GetInst ( )->GetStageImageName ( stageType );
    HFONT hFont = CreateUIFont ( 10 );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
    SetBkMode ( _dc , TRANSPARENT );

    RECT textRect = { x, y + m_iItemSize - 20, x + m_iItemSize, y + m_iItemSize };
    DrawTextW ( _dc , stageName , -1 , &textRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

    SelectObject ( _dc , hOldFont );
    SelectObject ( _dc , hOldPen );
    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hFont );
    DeleteObject ( hPen );
    DeleteObject ( hBrush );
}

void CEditorUI::RenderStageImageIcon ( HDC _dc , STAGE_IMAGE_TYPE stageType , int centerX , int centerY )
{
    COLORREF iconColor;
    const wchar_t* iconText;

    switch ( stageType )
    {
    case STAGE_IMAGE_TYPE::STAGE_01:
        iconColor = RGB ( 100 , 255 , 100 );  // 초록색 (Green Hill)
        iconText = L"S1";
        break;
    case STAGE_IMAGE_TYPE::STAGE_02:
        iconColor = RGB ( 150 , 150 , 150 );  // 회색 (Castle)
        iconText = L"S2";
        break;
    case STAGE_IMAGE_TYPE::CUSTOM:
        iconColor = RGB ( 255 , 255 , 100 );  // 노란색 (Custom)
        iconText = L"CU";
        break;
    default:
        iconColor = RGB ( 255 , 255 , 255 );
        iconText = L"??";
        break;
    }

    // 아이콘 배경 원
    HBRUSH hBrush = CreateSolidBrush ( iconColor );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , hBrush );
    Ellipse ( _dc , centerX - 15 , centerY - 15 , centerX + 15 , centerY + 15 );

    // 아이콘 텍스트
    HFONT hFont = CreateUIFont ( 12 , true );
    HFONT hOldFont = ( HFONT ) SelectObject ( _dc , hFont );
    SetTextColor ( _dc , RGB ( 0 , 0 , 0 ) );
    SetBkMode ( _dc , TRANSPARENT );

    RECT textRect = { centerX - 15, centerY - 8, centerX + 15, centerY + 8 };
    DrawTextW ( _dc , iconText , -1 , &textRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

    SelectObject ( _dc , hOldFont );
    SelectObject ( _dc , hOldBrush );
    DeleteObject ( hFont );
    DeleteObject ( hBrush );
}

void CEditorUI::RenderMonsterProperties ( HDC _dc , CMonster* _pMonster )
{
    int currentY = m_iPropertyPanelY + 40;  // 헤더 아래부터 시작
    int leftMargin = m_iPropertyPanelX + 10;
    int lineHeight = 25;

    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    // 오브젝트 타입 표시
    RenderBoldText ( _dc , leftMargin , currentY , L"Monster Object" , RGB ( 255 , 255 , 100 ) );
    currentY += lineHeight;

    // 몬스터 방향 설정 섹션
    RenderText ( _dc , leftMargin , currentY , L"Initial Direction:" );
    currentY += lineHeight;

    // 방향 버튼들
    int buttonWidth = 60;
    int buttonHeight = 25;
    int buttonSpacing = 10;
    int leftButtonX = leftMargin;
    int rightButtonX = leftMargin + buttonWidth + buttonSpacing;

    bool facingLeft = ( _pMonster->GetDirection ( ) < 0 );
    bool facingRight = ( _pMonster->GetDirection ( ) > 0 );

    // 왼쪽 버튼
    HBRUSH leftBrush = CreateSolidBrush ( facingLeft ? RGB ( 100 , 150 , 255 ) : RGB ( 70 , 70 , 70 ) );
    HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , leftBrush );
    Rectangle ( _dc , leftButtonX , currentY , leftButtonX + buttonWidth , currentY + buttonHeight );

    SetTextColor ( _dc , facingLeft ? RGB ( 255 , 255 , 255 ) : RGB ( 180 , 180 , 180 ) );
    RECT leftTextRect = { leftButtonX, currentY, leftButtonX + buttonWidth, currentY + buttonHeight };
    DrawTextW ( _dc , L"Left" , -1 , &leftTextRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

    // 오른쪽 버튼  
    HBRUSH rightBrush = CreateSolidBrush ( facingRight ? RGB ( 100 , 150 , 255 ) : RGB ( 70 , 70 , 70 ) );
    SelectObject ( _dc , rightBrush );
    Rectangle ( _dc , rightButtonX , currentY , rightButtonX + buttonWidth , currentY + buttonHeight );

    SetTextColor ( _dc , facingRight ? RGB ( 255 , 255 , 255 ) : RGB ( 180 , 180 , 180 ) );
    RECT rightTextRect = { rightButtonX, currentY, rightButtonX + buttonWidth, currentY + buttonHeight };
    DrawTextW ( _dc , L"Right" , -1 , &rightTextRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

    SelectObject ( _dc , hOldBrush );
    DeleteObject ( leftBrush );
    DeleteObject ( rightBrush );
}

void CEditorUI::RenderTileProperties ( HDC _dc , CTile* _pTile )
{
    int currentY = m_iPropertyPanelY + 40;  // 헤더 아래부터 시작
    int leftMargin = m_iPropertyPanelX + 10;
    int lineHeight = 25;

    SetBkMode ( _dc , TRANSPARENT );
    SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );

    // 오브젝트 타입 표시
    if ( _pTile->GetType ( ) == OBJECT_TYPE::TILE_TRIGGER )
    {
        RenderBoldText ( _dc , leftMargin , currentY , L"Trigger Block" , RGB ( 255 , 100 , 255 ) );
    }
    else
    {
        RenderBoldText ( _dc , leftMargin , currentY , L"Tile Object" , RGB ( 255 , 255 , 100 ) );
    }
    currentY += lineHeight;

    // 트리거 블록의 경우에만 카메라 좌표 설정 표시
    if ( _pTile->GetType ( ) == OBJECT_TYPE::TILE_TRIGGER )
    {
        // 카메라 잠금 좌표 섹션
        RenderText ( _dc , leftMargin , currentY , L"Boss Camera Lock Position:" );
        currentY += lineHeight;

        Vec2 vLockPos = _pTile->GetBossLockPosition ( );
        int buttonSize = 25;
        int valueWidth = 60;
        int buttonSpacing = 5;

        // X 좌표 조정
        wchar_t szXValue[32];
        swprintf_s ( szXValue , L"X: %.0f" , vLockPos.x );
        RenderText ( _dc , leftMargin , currentY , szXValue );
        
        int xButtonStartX = leftMargin + 80;
        
        // X- 버튼
        HBRUSH xMinusBrush = CreateSolidBrush ( RGB ( 200 , 100 , 100 ) );
        HBRUSH hOldBrush = ( HBRUSH ) SelectObject ( _dc , xMinusBrush );
        Rectangle ( _dc , xButtonStartX , currentY , xButtonStartX + buttonSize , currentY + buttonSize );
        SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
        RECT xMinusRect = { xButtonStartX, currentY, xButtonStartX + buttonSize, currentY + buttonSize };
        DrawTextW ( _dc , L"-" , -1 , &xMinusRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        // X+ 버튼
        HBRUSH xPlusBrush = CreateSolidBrush ( RGB ( 100 , 200 , 100 ) );
        SelectObject ( _dc , xPlusBrush );
        int xPlusX = xButtonStartX + buttonSize + buttonSpacing;
        Rectangle ( _dc , xPlusX , currentY , xPlusX + buttonSize , currentY + buttonSize );
        RECT xPlusRect = { xPlusX, currentY, xPlusX + buttonSize, currentY + buttonSize };
        DrawTextW ( _dc , L"+" , -1 , &xPlusRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        currentY += buttonSize + 10;

        // Y 좌표 조정
        wchar_t szYValue[32];
        swprintf_s ( szYValue , L"Y: %.0f" , vLockPos.y );
        RenderText ( _dc , leftMargin , currentY , szYValue );
        
        int yButtonStartX = leftMargin + 80;
        
        // Y- 버튼
        HBRUSH yMinusBrush = CreateSolidBrush ( RGB ( 200 , 100 , 100 ) );
        SelectObject ( _dc , yMinusBrush );
        Rectangle ( _dc , yButtonStartX , currentY , yButtonStartX + buttonSize , currentY + buttonSize );
        RECT yMinusRect = { yButtonStartX, currentY, yButtonStartX + buttonSize, currentY + buttonSize };
        DrawTextW ( _dc , L"-" , -1 , &yMinusRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        // Y+ 버튼
        HBRUSH yPlusBrush = CreateSolidBrush ( RGB ( 100 , 200 , 100 ) );
        SelectObject ( _dc , yPlusBrush );
        int yPlusX = yButtonStartX + buttonSize + buttonSpacing;
        Rectangle ( _dc , yPlusX , currentY , yPlusX + buttonSize , currentY + buttonSize );
        RECT yPlusRect = { yPlusX, currentY, yPlusX + buttonSize, currentY + buttonSize };
        DrawTextW ( _dc , L"+" , -1 , &yPlusRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        currentY += buttonSize + 15;

        // 추가 기능 버튼들
        int utilButtonWidth = 80;
        int utilButtonHeight = 20;

        // Reset 버튼
        HBRUSH resetBrush = CreateSolidBrush ( RGB ( 150 , 150 , 150 ) );
        SelectObject ( _dc , resetBrush );
        Rectangle ( _dc , leftMargin , currentY , leftMargin + utilButtonWidth , currentY + utilButtonHeight );
        SetTextColor ( _dc , RGB ( 255 , 255 , 255 ) );
        RECT resetRect = { leftMargin, currentY, leftMargin + utilButtonWidth, currentY + utilButtonHeight };
        DrawTextW ( _dc , L"Reset" , -1 , &resetRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        // Use Camera 버튼
        HBRUSH cameraBrush = CreateSolidBrush ( RGB ( 100 , 100 , 200 ) );
        SelectObject ( _dc , cameraBrush );
        int cameraButtonX = leftMargin + utilButtonWidth + 10;
        Rectangle ( _dc , cameraButtonX , currentY , cameraButtonX + utilButtonWidth + 20 , currentY + utilButtonHeight );
        RECT cameraRect = { cameraButtonX, currentY, cameraButtonX + utilButtonWidth + 20, currentY + utilButtonHeight };
        DrawTextW ( _dc , L"Use Camera" , -1 , &cameraRect , DT_CENTER | DT_VCENTER | DT_SINGLELINE );

        SelectObject ( _dc , hOldBrush );
        DeleteObject ( xMinusBrush );
        DeleteObject ( xPlusBrush );
        DeleteObject ( yMinusBrush );
        DeleteObject ( yPlusBrush );
        DeleteObject ( resetBrush );
        DeleteObject ( cameraBrush );
    }

    // 충돌 타입 정보 표시
    currentY += lineHeight + 10;
    RenderText ( _dc , leftMargin , currentY , L"Collision Type:" );
    currentY += 20;

    COLLISION_TYPE collisionType = _pTile->GetCollisionType ( );
    const wchar_t* collisionName = L"Unknown";
    
    switch ( collisionType )
    {
        case COLLISION_TYPE::SOLID_GROUND: collisionName = L"Solid Ground"; break;
        case COLLISION_TYPE::PLATFORM: collisionName = L"Platform"; break;
        case COLLISION_TYPE::SPIKE: collisionName = L"Spike"; break;
        case COLLISION_TYPE::WATER: collisionName = L"Water"; break;
        case COLLISION_TYPE::TRIGGER: collisionName = L"Trigger"; break;
        default: collisionName = L"Unknown"; break;
    }
    
    RenderText ( _dc , leftMargin , currentY , collisionName );
}

bool CEditorUI::HandleMonsterPropertyEdit ( CMonster* _pMonster , Vec2 _vPos )
{
    // 방향 버튼 클릭 체크
    int buttonY = m_iPropertyPanelY + 90; // 헤더(40) + "몬스터 오브젝트"(25) + "초기 방향:"(25) = 90
    int buttonWidth = 60;
    int buttonHeight = 25;
    int leftMargin = m_iPropertyPanelX + 10;
    int buttonSpacing = 10;

    int leftButtonX = leftMargin;
    int rightButtonX = leftMargin + buttonWidth + buttonSpacing;

    // 왼쪽 버튼 클릭 체크
    if ( _vPos.x >= leftButtonX && _vPos.x <= leftButtonX + buttonWidth &&
        _vPos.y >= buttonY && _vPos.y <= buttonY + buttonHeight )
    {
        _pMonster->SetDirection ( -1 ); // 왼쪽 방향
        return true;
    }

    // 오른쪽 버튼 클릭 체크
    if ( _vPos.x >= rightButtonX && _vPos.x <= rightButtonX + buttonWidth &&
        _vPos.y >= buttonY && _vPos.y <= buttonY + buttonHeight )
    {
        _pMonster->SetDirection ( 1 ); // 오른쪽 방향
        return true;
    }

    return false;
}

bool CEditorUI::HandleTilePropertyEdit ( CTile* _pTile , Vec2 _vPos )
{
    // 트리거 블록이 아닌 경우는 편집할 내용이 없음
    if ( _pTile->GetType ( ) != OBJECT_TYPE::TILE_TRIGGER )
        return false;

    int leftMargin = m_iPropertyPanelX + 10;
    int buttonSize = 25;
    int buttonSpacing = 5;
    
    // UI 레이아웃 계산
    // 헤더(40) + "Trigger Block"(25) + "Boss Camera Lock Position:"(25) = 90
    int xControlY = m_iPropertyPanelY + 90;
    int yControlY = xControlY + buttonSize + 10;  // X 컨트롤 아래 + 간격
    int utilButtonY = yControlY + buttonSize + 15; // Y 컨트롤 아래 + 간격
    
    int xButtonStartX = leftMargin + 80;
    int yButtonStartX = leftMargin + 80;
    
    Vec2 vCurrentPos = _pTile->GetBossLockPosition ( );
    const float GRID_SIZE = 32.0f;
    const float MIN_COORD = 0.0f;
    const float MAX_X = 1600.0f;
    const float MAX_Y = 1200.0f;
    
    // X- 버튼 클릭
    if ( _vPos.x >= xButtonStartX && _vPos.x <= xButtonStartX + buttonSize &&
        _vPos.y >= xControlY && _vPos.y <= xControlY + buttonSize )
    {
        float newX = max ( MIN_COORD , vCurrentPos.x - GRID_SIZE );
        _pTile->SetBossLockPosition ( Vec2 ( newX , vCurrentPos.y ) );
        return true;
    }
    
    // X+ 버튼 클릭
    int xPlusX = xButtonStartX + buttonSize + buttonSpacing;
    if ( _vPos.x >= xPlusX && _vPos.x <= xPlusX + buttonSize &&
        _vPos.y >= xControlY && _vPos.y <= xControlY + buttonSize )
    {
        float newX = min ( MAX_X , vCurrentPos.x + GRID_SIZE );
        _pTile->SetBossLockPosition ( Vec2 ( newX , vCurrentPos.y ) );
        return true;
    }
    
    // Y- 버튼 클릭
    if ( _vPos.x >= yButtonStartX && _vPos.x <= yButtonStartX + buttonSize &&
        _vPos.y >= yControlY && _vPos.y <= yControlY + buttonSize )
    {
        float newY = max ( MIN_COORD , vCurrentPos.y - GRID_SIZE );
        _pTile->SetBossLockPosition ( Vec2 ( vCurrentPos.x , newY ) );
        return true;
    }
    
    // Y+ 버튼 클릭
    int yPlusX = yButtonStartX + buttonSize + buttonSpacing;
    if ( _vPos.x >= yPlusX && _vPos.x <= yPlusX + buttonSize &&
        _vPos.y >= yControlY && _vPos.y <= yControlY + buttonSize )
    {
        float newY = min ( MAX_Y , vCurrentPos.y + GRID_SIZE );
        _pTile->SetBossLockPosition ( Vec2 ( vCurrentPos.x , newY ) );
        return true;
    }
    
    // Reset 버튼 클릭
    int utilButtonWidth = 80;
    int utilButtonHeight = 20;
    if ( _vPos.x >= leftMargin && _vPos.x <= leftMargin + utilButtonWidth &&
        _vPos.y >= utilButtonY && _vPos.y <= utilButtonY + utilButtonHeight )
    {
        _pTile->SetBossLockPosition ( Vec2 ( 400.0f , 300.0f ) ); // 기본값
        return true;
    }
    
    // Use Camera 버튼 클릭
    int cameraButtonX = leftMargin + utilButtonWidth + 10;
    int cameraButtonWidth = utilButtonWidth + 20;
    if ( _vPos.x >= cameraButtonX && _vPos.x <= cameraButtonX + cameraButtonWidth &&
        _vPos.y >= utilButtonY && _vPos.y <= utilButtonY + utilButtonHeight )
    {
        // 현재 카메라 위치를 가져와서 설정 (카메라 시스템 추가 필요)
        // 임시로 현재 위치를 그리드에 맞춰서 설정
        float gridX = floor ( vCurrentPos.x / GRID_SIZE ) * GRID_SIZE;
        float gridY = floor ( vCurrentPos.y / GRID_SIZE ) * GRID_SIZE;
        _pTile->SetBossLockPosition ( Vec2 ( gridX , gridY ) );
        return true;
    }
    
    return false;
}