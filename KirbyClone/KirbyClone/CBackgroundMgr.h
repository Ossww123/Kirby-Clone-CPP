#pragma once

class CBackground;
class CTexture;

class CBackgroundMgr
{
    SINGLE(CBackgroundMgr);

private:
    map<BACKGROUND_TYPE, CBackground*> m_mapBackground;   // 배경 맵

public:
    void init();

    // 배경 생성 및 관리
    CBackground* CreateBackground(BACKGROUND_TYPE _eType, const wstring& _strTexturePath);
    CBackground* FindBackground(BACKGROUND_TYPE _eType);

    // 배경 타입 관련 유틸리티
    const wchar_t* GetBackgroundName(BACKGROUND_TYPE _eType);
    vector<BACKGROUND_TYPE> GetAvailableBackgroundTypes();

private:
    void CreateDefaultBackgrounds();    // 기본 배경들 생성
};