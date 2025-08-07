#pragma once

class CStageImage;
class CTexture;

// 스테이지 이미지들을 관리하는 매니저 클래스
class CStageMgr
{
    SINGLE(CStageMgr);

private:
    map<STAGE_IMAGE_TYPE, CStageImage*> m_mapStageImage;   // 스테이지 이미지 맵
    CStageImage* m_pCurrentStageImage;                     // 현재 선택된 스테이지 이미지

public:
    void init();

    // 스테이지 이미지 생성 및 관리
    CStageImage* CreateStageImage(STAGE_IMAGE_TYPE _eType, const wstring& _strTexturePath);
    CStageImage* FindStageImage(STAGE_IMAGE_TYPE _eType);
    void SetCurrentStageImage(STAGE_IMAGE_TYPE _eType);

    // 현재 스테이지 이미지 관련
    CStageImage* GetCurrentStageImage() { return m_pCurrentStageImage; }
    STAGE_IMAGE_TYPE GetCurrentStageType();

    // 스테이지 이미지 타입 관련 유틸리티
    const wchar_t* GetStageImageName(STAGE_IMAGE_TYPE _eType);
    vector<STAGE_IMAGE_TYPE> GetAvailableStageImageTypes();

    // 사용자 커스텀 스테이지 이미지 로드
    bool LoadCustomStageImage(const wstring& _strFilePath);

    // 렌더링
    void Update();                      // 현재 스테이지 이미지 업데이트
    void Render(HDC _dc);              // 현재 스테이지 이미지 렌더링

private:
    void CreateDefaultStageImages();    // 기본 스테이지 이미지들 생성
    STAGE_IMAGE_TYPE GenerateCustomStageType(); // 커스텀 스테이지용 새로운 타입 생성
};