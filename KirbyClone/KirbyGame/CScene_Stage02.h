#pragma once
#include "CScene.h"

class CScene_Stage02 : public CScene
{
public:
    CScene_Stage02();
    ~CScene_Stage02();

public:
    // === 핵심 생명주기 함수들 ===
    void Enter() override;
    void Exit() override;
    void Update() override;
    void Render(HDC _dc) override;

private:
    // === 레벨 로드 관련 함수들 ===
    void LoadStageLevel(const wstring& _strFileName);
    CObject* CreateObjectFromData(const tLevelObjectData& _objData);
    void CreateDefaultLevel();
    void ApplyLoadedLevelData(Vec2 _vPlayerSpawn, BACKGROUND_TYPE _eBgType, STAGE_IMAGE_TYPE _eStageType);

private:
    // === 스테이지별 초기 설정 ===
    void InitializeStage();

private:

private:
    // === 멤버 변수들 ===

    // === 레벨 파일 관리 ===
    wstring         m_strLevelFile;         // 로드할 레벨 파일명

};