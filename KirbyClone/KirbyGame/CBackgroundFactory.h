#pragma once

class CObject;
class CBackground;
class CTexture;

class CBackgroundFactory
{
private:
    // 팩토리는 정적 클래스로 사용
    CBackgroundFactory() = delete;
    ~CBackgroundFactory() = delete;

public:
    // 배경 생성 함수
    static CObject* CreateBackground(BACKGROUND_TYPE _eBackgroundType, Vec2 _vPos);

    // === 배경별 세부 생성 함수들 ===
    static CBackground* CreateStaticBackground(Vec2 _vPos);
    static CBackground* CreateScrollableBackground(Vec2 _vPos);
    static CBackground* CreateParallaxBackground(Vec2 _vPos);

    // === 특정 배경 이미지로 생성 ===
    static CObject* CreateBackgroundWithTexture(BACKGROUND_TYPE _eType, const wstring& _strTexturePath, Vec2 _vPos);

    // 배경 관련 유틸리티 함수들
    static const wchar_t* GetBackgroundTypeName(BACKGROUND_TYPE _eType);
    static GROUP_TYPE GetBackgroundGroup(BACKGROUND_TYPE _eType);
    static Vec2 GetBackgroundDefaultScale(BACKGROUND_TYPE _eType);
    static bool IsBackgroundType(BACKGROUND_TYPE _eType);

    // 배경 텍스처 경로 매핑
    static const wchar_t* GetBackgroundTexturePath(BACKGROUND_TYPE _eType);

private:
    // 내부 설정 함수
    static void SetupBackgroundProperties(CBackground* _pBackground, BACKGROUND_TYPE _eType);
};