#pragma once

// 전방 선언
class CStageImage;
class CTexture;

// 스테이지 이미지들을 관리하는 매니저 클래스
class CStageMgr
{
    SINGLE(CStageMgr);

public:
    // === 핵심 생명주기 함수 ===
    void init();
    void Update();
    void Render(HDC _dc);

public:
    // === 스테이지 이미지 생성 및 관리 ===
    CStageImage* CreateStageImage(STAGE_IMAGE_TYPE _eType, const wstring& _strTexturePath);
    CStageImage* FindStageImage(STAGE_IMAGE_TYPE _eType) const;
    void SetCurrentStageImage(STAGE_IMAGE_TYPE _eType);

private:
    // === 스테이지 생성 관련 내부 처리 ===
    void CreateDefaultStageImages();

public:
    // === 현재 스테이지 이미지 접근자 ===
    CStageImage* GetCurrentStageImage() const { return m_pCurrentStageImage; }
    STAGE_IMAGE_TYPE GetCurrentStageType() const;

public:
    // === 스테이지 정보 유틸리티 ===
    const wchar_t* GetStageImageName(STAGE_IMAGE_TYPE _eType) const;
    vector<STAGE_IMAGE_TYPE> GetAvailableStageImageTypes() const;

private:
    // === 멤버 변수들 ===
    map<STAGE_IMAGE_TYPE, CStageImage*> m_mapStageImage;    // 스테이지 이미지 맵
    CStageImage* m_pCurrentStageImage; // 현재 선택된 스테이지 이미지
};