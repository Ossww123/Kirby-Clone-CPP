#pragma once

class CEditorCore;

class CEditorInput
{
private:
    CEditorCore* m_pEditorCore;

public:
    void Initialize(CEditorCore* _pCore);
    void Update();

private:
    // 입력 처리 세분화
    void UpdateGeneralInput();      // 일반 입력 (UI 토글 등)
    void UpdateGridInput();         // 그리드 관련 입력
    void UpdateModeInput();         // 모드 전환 입력
    void UpdateMouseInput();        // 마우스 입력
    void UpdateObjectSelection();   // 오브젝트 선택 입력
    void UpdateFileInput();         // 파일 관련 입력

    // 마우스 처리
    void HandleMouseClick();
    void UpdateMousePosition();

    // 모드별 입력 처리
    void HandleModeSpecificInput();
    void HandleBackgroundModeInput();
    void HandleTileModeInput();
    void HandleStageImageClick();
    void HandleStageImageModeInput();

    void ResetStageImageToBottomLeft();

    void PrevStageImage();
    void NextStageImage();
    void LoadCustomStageImage();
public:
    CEditorInput();
    ~CEditorInput();
};