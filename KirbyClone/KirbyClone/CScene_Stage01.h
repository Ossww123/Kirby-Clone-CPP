#pragma once
#include "CScene.h"

class CScene_Stage01 : public CScene
{
private:
    wstring m_strLevelFile;     // 로드할 레벨 파일명

public:
    virtual void Enter();
    virtual void Exit();
    virtual void Update();

private:
    // 레벨 로드 관련 함수들
    void LoadStageLevel(const wstring& _strFileName);
    CObject* CreateObjectFromData(const tLevelObjectData& _objData);
    void CreateDefaultLevel();

    // 스테이지별 초기 설정
    void InitializeStage();

public:
    CScene_Stage01();
    ~CScene_Stage01();
};