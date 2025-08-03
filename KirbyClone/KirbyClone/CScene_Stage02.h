#pragma once
#include "CScene.h"

class CBackground;

class CScene_Stage02 : public CScene
{
private:
    wstring         m_strLevelFile;         // 로드할 레벨 파일명
    CBackground* m_pCurrentBackground;   // 현재 배경
    BACKGROUND_TYPE m_eCurrentBgType;       // 현재 배경 타입

public:
    virtual void Enter();
    virtual void Exit();
    virtual void Update();
    virtual void Render(HDC _dc);

private:
    // 레벨 로드 관련 함수들
    void LoadStageLevel(const wstring& _strFileName);
    CObject* CreateObjectFromData(const tLevelObjectData& _objData);
    void CreateDefaultLevel();

    // 스테이지별 초기 설정
    void InitializeStage();

    // 배경 시스템 관련 함수들
    void InitializeBackgroundSystem();    // 배경 시스템 초기화
    void ChangeBackground(BACKGROUND_TYPE _eBgType);  // 배경 변경

public:
    CScene_Stage02();
    ~CScene_Stage02();
};