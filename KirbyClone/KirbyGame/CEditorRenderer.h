#pragma once

class CEditorCore;
class CAnimation;
class CObject;
class CMonster;

class CEditorRenderer
{
private:
    CEditorCore* m_pEditorCore;

public:
    void Initialize ( CEditorCore* _pCore );
    void Render ( HDC _dc );

    // 특정 요소별 렌더링
    void RenderMouse ( HDC _dc );
    void RenderPreview ( HDC _dc );
    void RenderSelectedObject ( HDC _dc );
    void RenderPlayerSpawnPoint ( HDC _dc );
    void RenderMonsterDirectionArrow ( HDC _dc , CMonster* _pMonster );

private:
    // 렌더링 유틸리티
    void RenderMouseCursor ( HDC _dc , Vec2 vRenderPos , COLORREF color );
    void RenderPreviewObject ( HDC _dc , Vec2 vRenderPos , Vec2 vObjectSize , COLORREF color );
    void RenderDeletePreview ( HDC _dc , CObject* pTargetObj );
    void RenderSelectionBox ( HDC _dc , CObject* pObj );
    void RenderGridPreview ( HDC _dc , Vec2 vRenderPos );
    void RenderGameOverLine ( HDC _dc );
    void RenderLeftBoundaryLine ( HDC _dc );

    // 색상 관리
    COLORREF GetModeColor ( );
    COLORREF GetPreviewColor ( );

    // 그리기 보조 함수들
    void DrawCross ( HDC _dc , Vec2 vPos , int size , COLORREF color , int thickness = 2 );
    void DrawX ( HDC _dc , Vec2 vPos , int size , COLORREF color , int thickness = 3 );
    void DrawDottedRectangle ( HDC _dc , Vec2 vPos , Vec2 vSize , COLORREF color );
    void DrawTextWithBackground ( HDC _dc , Vec2 vPos , const wchar_t* text , COLORREF textColor , COLORREF bgColor = RGB ( 0 , 0 , 0 ) );

public:
    CEditorRenderer ( );
    ~CEditorRenderer ( );
};