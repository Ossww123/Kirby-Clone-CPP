#pragma once
#include "CScene.h"
#include "CObjectFactory.h"

class CScene_Tool : public CScene
{
private:
    bool m_bShowUI;         // UI 표시 여부
    Vec2 m_vMousePos;       // 마우스 월드 좌표
    bool m_bMouseClick;     // 마우스 클릭 상태
    float m_fClickTime;     // 클릭 표시 시간

    // 에디터 모드 관련
    EDITOR_MODE m_eCurrentMode;     // 현재 에디터 모드

    // 오브젝트 팩토리 관련
    OBJECT_TYPE         m_eCurrentObjectType;   // 현재 선택된 오브젝트 타입
    int                 m_iCurrentSubType;      // 현재 카테고리 내 서브타입 인덱스
    vector<OBJECT_TYPE> m_vecCurrentCategory;   // 현재 카테고리의 오브젝트 타입들

    // 선택 도구 관련
    CObject* m_pSelectedObject;     // 선택된 오브젝트
    bool m_bDragging;              // 드래그 중인지 여부
    Vec2 m_vDragStartPos;          // 드래그 시작 위치

public:
    virtual void Enter();
    virtual void Exit();
    virtual void Update();
    virtual void Render(HDC _dc);

private:
    void UpdateInput();     // 입력 처리
    void UpdateMouse();     // 마우스 입력 처리
    void UpdateModeInput(); // 모드 전환 입력 처리
    void UpdateGridInput(); // 그리드 관련 입력 처리
    void UpdateObjectSelection(); // 오브젝트 선택 입력 처리
    void UpdateCameraMove(); // 카메라 이동 처리
    void HandleMouseClick(); // 마우스 클릭 처리
    void RenderUI(HDC _dc); // UI 렌더링
    void RenderMouse(HDC _dc); // 마우스 커서 렌더링
    void RenderPreview(HDC _dc); // 배치 미리보기 렌더링
    void RenderSelectedObject(HDC _dc); // 선택된 오브젝트 하이라이트

    // 오브젝트 관리 함수 (팩토리 사용)
    void PlaceObject(Vec2 _vPos);               // 현재 선택된 타입의 오브젝트 배치

    CObject* FindObjectAtPosition(Vec2 _vPos);  // 위치에서 오브젝트 찾기
    void SetSelectedObject(CObject* _pObj);     // 오브젝트 선택
    void DeselectObject();                      // 선택 해제
    void DeleteObjectAtPosition(Vec2 _vPos);    // 위치의 오브젝트 삭제

    // 오브젝트 타입 관리
    void ChangeObjectCategory(const wstring& _strCategory); // 카테고리 변경
    void NextObjectInCategory();                // 카테고리 내 다음 오브젝트
    void PrevObjectInCategory();                // 카테고리 내 이전 오브젝트
    const wchar_t* GetCurrentObjectName();      // 현재 선택된 오브젝트 이름

    // 저장/로딩 함수
    void SaveLevel(const wstring& _strFileName);    // 레벨 저장
    void LoadLevel(const wstring& _strFileName);    // 레벨 로딩
    void QuickSave();                               // 빠른 저장 (F5)
    void QuickLoad();                               // 빠른 로딩 (F9)
    void SaveAsDialog();                            // 다른 이름으로 저장 (Ctrl+S)
    void OpenDialog();                              // 파일 열기 (Ctrl+O)

    // 유틸리티 함수
    void ChangeMode(EDITOR_MODE _eMode);    // 모드 변경
    const wchar_t* GetModeString();         // 모드 이름 문자열 반환

public:
    CScene_Tool();
    ~CScene_Tool();
};