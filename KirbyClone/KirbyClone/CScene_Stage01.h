#pragma once
#include "CScene.h"

class CBackground;

class CScene_Stage01 : public CScene
{
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
    // === 배경 시스템 관련 함수들 ===
    void InitializeBackgroundSystem();
    void ChangeBackground(BACKGROUND_TYPE _eBgType);

public:
    // === 생성자/소멸자 ===
    CScene_Stage01();
    ~CScene_Stage01();

private:
    // === 멤버 변수들 ===

    // === 레벨 파일 관리 ===
    wstring         m_strLevelFile;         // 로드할 레벨 파일명

    // === 배경 시스템 ===
    CBackground* m_pCurrentBackground;   // 현재 배경
    BACKGROUND_TYPE m_eCurrentBgType;       // 현재 배경 타입
};