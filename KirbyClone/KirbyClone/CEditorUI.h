#pragma once

class CEditorCore;
class CScene;

class CEditorUI
{
private:
    CEditorCore* m_pEditorCore;
    CScene* m_pScene;

    // UI 설정
    int m_iUIWidth;
    int m_iUIHeight;
    int m_iUIMargin;
    int m_iLineHeight;

    // 객체 팔레트 관련
    int m_iPaletteX;
    int m_iPaletteY;
    int m_iPaletteWidth;
    int m_iPaletteHeight;
    int m_iItemSize;
    int m_iItemPadding;
    int m_iItemsPerRow;

    // 스크롤 관련
    int m_iScrollOffset;
    int m_iMaxScroll;

public:
    void Initialize(CEditorCore* _pCore, CScene* _pScene);
    void Render(HDC _dc);

private:
    // UI 렌더링 세분화
    void RenderMainUI(HDC _dc);
    void RenderModeInfo(HDC _dc, int& yPos);
    void RenderBackgroundSettings(HDC _dc, int& yPos);
    void RenderObjectInfo(HDC _dc, int& yPos);
    void RenderTileVisualSettings(HDC _dc, int& yPos);
    void RenderGridInfo(HDC _dc, int& yPos);
    void RenderObjectCount(HDC _dc, int& yPos);
    void RenderControlInstructions(HDC _dc, int& yPos);
    void RenderBackgroundModeUI(HDC _dc, int& yPos);
    void RenderLevelBounds(HDC _dc, int& yPos);

    // UI 구성 요소 렌더링
    void RenderUIHeader(HDC _dc, int& yPos);
    void RenderSeparatorLine(HDC _dc, int yPos);
    void RenderHighlightBox(HDC _dc, int x, int y, int width, int height);

    // 팔레트 세부 렌더링
    void RenderPaletteBackground(HDC _dc);
    void RenderPaletteHeader(HDC _dc);
    void RenderPaletteItems(HDC _dc);
    void RenderPaletteItem(HDC _dc, int index, OBJECT_TYPE objType, int x, int y, bool selected);
    void RenderObjectIcon(HDC _dc, OBJECT_TYPE objType, int x, int y, int size);

    // 새로 추가: 충돌체 관련 렌더링 함수들
    void RenderCollisionIcon(HDC _dc, COLLISION_TYPE collisionType, int centerX, int centerY);
    void RenderCollisionPalette(HDC _dc);                    // 충돌체 전용 팔레트 (선택사항)
    void RenderStageImagePalette(HDC _dc);                   // 스테이지 이미지 팔레트 (선택사항)
    void RenderCollisionTooltip(HDC _dc, OBJECT_TYPE objType, int mouseX, int mouseY);
    void RenderStageImageItem(HDC _dc, STAGE_IMAGE_TYPE stageType, int x, int y, bool selected);
    void RenderStageImageIcon(HDC _dc, STAGE_IMAGE_TYPE stageType, int centerX, int centerY);

    // UI 유틸리티
    void DrawUIBackground(HDC _dc);
    void SetupTextStyle(HDC _dc, COLORREF color);
    void RenderText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color = RGB(255, 255, 255));
    void RenderBoldText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color = RGB(255, 255, 100));

public:
    // 객체 팔레트 렌더링
    void RenderObjectPalette(HDC _dc);
    bool HandlePaletteClick(Vec2 vMousePos);
    bool HandleStageImagePaletteClick(Vec2 vMousePos);
    //void UpdatePaletteScroll(int deltaY);

    // 팔레트 유틸리티
    int GetPaletteItemAt(Vec2 vMousePos);
    bool IsInPaletteArea(Vec2 vMousePos);
    void CalculatePaletteLayout();

    // 폰트 관리
    HFONT CreateUIFont(int size = 14, bool bold = false);

public:
    CEditorUI();
    ~CEditorUI();
};