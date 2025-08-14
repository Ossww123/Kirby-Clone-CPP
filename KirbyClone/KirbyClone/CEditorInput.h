#pragma once

class CEditorCore;
class CDoor;

class CEditorInput
{
public:
    // === 생명주기 함수 ===
    CEditorInput();
    ~CEditorInput();

public:
    // === 핵심 생명주기 함수 ===
    void Initialize(CEditorCore* _pCore);
    void Update();

private:
    // === 입력 처리 세분화 ===
    void UpdateGeneralInput();      // 일반 입력 (UI 토글 등)
    void UpdateFileInput();         // 파일 관련 입력
    void UpdateGridInput();         // 그리드 관련 입력
    void UpdateModeInput();         // 모드 전환 입력
    void UpdateSelectedObjectInput(); // 선택된 오브젝트 편집
    void UpdateMouseInput();        // 마우스 입력
    void UpdateObjectSelection();   // 오브젝트 선택 입력

public:
    // === 마우스 처리 ===
    void HandleMouseClick();
    void UpdateMousePosition();

private:
    // === 마우스 처리 내부 함수 ===
    // (HandleMouseClick에서 사용하는 내부 로직들)

public:
    // === 모드별 입력 처리 ===
    void HandleModeSpecificInput();
    void HandleBackgroundModeInput();
    void HandleTileModeInput();
    void HandleStageImageModeInput();
    void HandleDoorPropertyInput(CDoor* _pDoor);

private:
    void ShowDoorPropertyChanged(CDoor* _pDoor, const wchar_t* _message);

private:
    // === 스테이지 이미지 처리 ===
    void HandleStageImageClick();
    void ResetStageImageToBottomLeft();
    void PrevStageImage();
    void NextStageImage();
    void LoadCustomStageImage();

private:
    // === 멤버 변수들 ===

    // === 에디터 코어 참조 ===
    CEditorCore* m_pEditorCore;     // 에디터 코어 참조
};