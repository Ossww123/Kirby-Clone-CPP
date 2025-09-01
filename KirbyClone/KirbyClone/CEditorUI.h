#pragma once

class CEditorCore;
class CScene;
class CDoor;
class CMonster;

class CEditorUI
{
public:
    // === 기본 생명주기 함수 ===
    CEditorUI ( );
    ~CEditorUI ( );

    void Initialize ( CEditorCore* _pCore , CScene* _pScene );
    void Render ( HDC _dc );

public:
    // === 오브젝트 팔레트 시스템 ===
    void RenderObjectPalette ( HDC _dc );
    bool HandlePaletteClick ( Vec2 vMousePos );
    bool HandleStageImagePaletteClick ( Vec2 vMousePos );
    bool IsInPaletteArea ( Vec2 vMousePos ) const;

public:
    // === 속성 패널 시스템 ===
    void RenderPropertyPanel ( HDC _dc );
    bool HandlePropertyPanelClick ( Vec2 vMousePos );
    bool IsInPropertyPanelArea ( Vec2 vMousePos ) const;

private:
    // === 속성 패널 렌더링 ===
    void RenderPropertyPanelBackground ( HDC _dc );
    void RenderPropertyPanelHeader ( HDC _dc );
    void RenderDoorProperties ( HDC _dc , CDoor* _pDoor );
    void RenderMonsterProperties ( HDC _dc , CMonster* _pMonster );
    void RenderTileProperties ( HDC _dc , class CTile* _pTile );
    void RenderNoSelection ( HDC _dc );

private:
    // === 각 속성 편집 ===
    bool HandleDoorPropertyEdit ( CDoor* _pDoor , Vec2 _vPos );
    bool HandleMonsterPropertyEdit ( CMonster* _pMonster , Vec2 _vPos );
    bool HandleTilePropertyEdit ( class CTile* _pTile , Vec2 _vPos );
    void RenderSceneDropdown ( HDC _dc , SCENE_TYPE _currentScene , int _x , int _y , int _width , int _height );
    void RenderInputField ( HDC _dc , const wchar_t* _label , float _value , int _x , int _y , int _width );

private:
    // === 씬 이름 변환 유틸리티 ===
    const wchar_t* GetSceneName ( SCENE_TYPE _eScene ) const;

private:
    // === 팔레트 렌더링 ===
    void RenderPaletteBackground ( HDC _dc );
    void RenderPaletteHeader ( HDC _dc );
    void RenderPaletteItems ( HDC _dc );
    void RenderPaletteItem ( HDC _dc , int index , OBJECT_TYPE objType , int x , int y , bool selected );

    // === 아이콘 렌더링 ===
    void RenderObjectIcon ( HDC _dc , OBJECT_TYPE objType , int x , int y , int size );
    void RenderCollisionIcon ( HDC _dc , COLLISION_TYPE collisionType , int centerX , int centerY );

    // === 특수 팔레트 렌더링 ===
    void RenderCollisionPalette ( HDC _dc );
    void RenderStageImagePalette ( HDC _dc );
    void RenderStageImageItem ( HDC _dc , STAGE_IMAGE_TYPE stageType , int x , int y , bool selected );
    void RenderStageImageIcon ( HDC _dc , STAGE_IMAGE_TYPE stageType , int centerX , int centerY );

    // === 팔레트 유틸리티 ===
    int GetPaletteItemAt ( Vec2 vMousePos ) const;
    void CalculatePaletteLayout ( );

public:
    // === 텍스트 렌더링 유틸리티 ===
    void SetupTextStyle ( HDC _dc , COLORREF color );
    void RenderText ( HDC _dc , int x , int y , const wchar_t* text , COLORREF color = RGB ( 255 , 255 , 255 ) );
    void RenderBoldText ( HDC _dc , int x , int y , const wchar_t* text , COLORREF color = RGB ( 255 , 255 , 100 ) );

private:
    // === 폰트 관리 ===
    HFONT CreateUIFont ( int size = 14 , bool bold = false );

    // === 툴팁 시스템 ===
    void RenderCollisionTooltip ( HDC _dc , OBJECT_TYPE objType , int mouseX , int mouseY );

private:
    // === 핵심 시스템 연결 ===
    CEditorCore* m_pEditorCore;
    CScene* m_pScene;

    // === 팔레트 레이아웃 ===
    int                 m_iPaletteX;
    int                 m_iPaletteY;
    int                 m_iPaletteWidth;
    int                 m_iPaletteHeight;
    int                 m_iItemSize;
    int                 m_iItemPadding;
    int                 m_iItemsPerRow;

    // === 속성 패널 레이아웃 ===
    int                 m_iPropertyPanelX;
    int                 m_iPropertyPanelY;
    int                 m_iPropertyPanelWidth;      // 속성 패널 너비 (팔레트와 분리)
    int                 m_iPropertyPanelHeight;

    // === 스크롤 관리 ===
    int                 m_iScrollOffset;
    int                 m_iMaxScroll;
};