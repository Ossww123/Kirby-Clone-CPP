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

    // UI 구성 요소 렌더링
    void RenderUIHeader(HDC _dc, int& yPos);
    void RenderSeparatorLine(HDC _dc, int yPos);
    void RenderHighlightBox(HDC _dc, int x, int y, int width, int height);

    // UI 유틸리티
    void DrawUIBackground(HDC _dc);
    void SetupTextStyle(HDC _dc, COLORREF color);
    void RenderText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color = RGB(255, 255, 255));
    void RenderBoldText(HDC _dc, int x, int y, const wchar_t* text, COLORREF color = RGB(255, 255, 100));

    // 폰트 관리
    HFONT CreateUIFont(int size = 14, bool bold = false);

public:
    CEditorUI();
    ~CEditorUI();
};