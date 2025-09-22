#include "gamePCH.h"
#include "CBackgroundFactory.h"
#include "CBackground.h"
#include "CResMgr.h"
#include "CTexture.h"

CObject* CBackgroundFactory::CreateBackground(BACKGROUND_TYPE _eBackgroundType, Vec2 _vPos)
{
    CObject* pBackground = nullptr;

    // 각 배경별 세부 생성 함수 호출
    switch (_eBackgroundType)
    {
    case BACKGROUND_TYPE::STATIC:
        pBackground = CreateStaticBackground(_vPos);
        break;
    case BACKGROUND_TYPE::SCROLLABLE:
        pBackground = CreateScrollableBackground(_vPos);
        break;
    case BACKGROUND_TYPE::PARALLAX:
        pBackground = CreateParallaxBackground(_vPos);
        break;
    case BACKGROUND_TYPE::BACKGROUND1:
        pBackground = CreateBackgroundWithTexture(BACKGROUND_TYPE::BACKGROUND1, L"background\\background1.bmp", _vPos);
        break;
    case BACKGROUND_TYPE::BACKGROUND2:
        pBackground = CreateBackgroundWithTexture(BACKGROUND_TYPE::BACKGROUND2, L"background\\background2.bmp", _vPos);
        break;
    case BACKGROUND_TYPE::BACKGROUND3:
        pBackground = CreateBackgroundWithTexture(BACKGROUND_TYPE::BACKGROUND3, L"background\\background3.bmp", _vPos);
        break;
    default:
        return nullptr;
    }

    return pBackground;
}

CBackground* CBackgroundFactory::CreateStaticBackground(Vec2 _vPos)
{
    CBackground* pBackground = new CBackground;
    pBackground->SetPos(_vPos);

    // 정적 배경 설정
    pBackground->SetupBackground(BACKGROUND_TYPE::STATIC);

    return pBackground;
}

CBackground* CBackgroundFactory::CreateScrollableBackground(Vec2 _vPos)
{
    CBackground* pBackground = new CBackground;
    pBackground->SetPos(_vPos);

    // 스크롤 가능한 배경 설정
    pBackground->SetupBackground(BACKGROUND_TYPE::SCROLLABLE);

    return pBackground;
}

CBackground* CBackgroundFactory::CreateParallaxBackground(Vec2 _vPos)
{
    CBackground* pBackground = new CBackground;
    pBackground->SetPos(_vPos);

    // 패럴랙스 배경 설정
    pBackground->SetupBackground(BACKGROUND_TYPE::PARALLAX);

    return pBackground;
}

CObject* CBackgroundFactory::CreateBackgroundWithTexture(BACKGROUND_TYPE _eType, const wstring& _strTexturePath, Vec2 _vPos)
{
    CBackground* pBackground = new CBackground;
    pBackground->SetPos(_vPos);

    // 텍스처 로드
    wstring strTexName = L"Background_" + to_wstring((int)_eType);
    CTexture* pTexture = CResMgr::GetInst()->LoadTexture(strTexName, _strTexturePath);

    if (pTexture)
    {
        pBackground->SetBackgroundTexture(pTexture);
    }

    // 배경 타입 설정
    pBackground->SetBackgroundType(_eType);
    SetupBackgroundProperties(pBackground, _eType);

    return pBackground;
}

const wchar_t* CBackgroundFactory::GetBackgroundTypeName(BACKGROUND_TYPE _eType)
{
    switch (_eType)
    {
    case BACKGROUND_TYPE::STATIC: return L"Static Background";
    case BACKGROUND_TYPE::SCROLLABLE: return L"Scrollable Background";
    case BACKGROUND_TYPE::PARALLAX: return L"Parallax Background";
    case BACKGROUND_TYPE::BACKGROUND1: return L"Background1";
    case BACKGROUND_TYPE::BACKGROUND2: return L"Background2";
    case BACKGROUND_TYPE::BACKGROUND3: return L"Background3";
    default: return L"Unknown Background";
    }
}

GROUP_TYPE CBackgroundFactory::GetBackgroundGroup(BACKGROUND_TYPE _eType)
{
    return GROUP_TYPE::DEFAULT;  // 배경은 기본 그룹에 속함
}

Vec2 CBackgroundFactory::GetBackgroundDefaultScale(BACKGROUND_TYPE _eType)
{
    // 모든 배경은 화면 해상도 크기
    return Vec2(640.f, 480.f);  // 기본 해상도
}

bool CBackgroundFactory::IsBackgroundType(BACKGROUND_TYPE _eType)
{
    return _eType >= BACKGROUND_TYPE::STATIC && _eType < BACKGROUND_TYPE::END;
}

const wchar_t* CBackgroundFactory::GetBackgroundTexturePath(BACKGROUND_TYPE _eType)
{
    switch (_eType)
    {
    case BACKGROUND_TYPE::BACKGROUND1: return L"background\\background1.bmp";
    case BACKGROUND_TYPE::BACKGROUND2: return L"background\\background2.bmp";
    case BACKGROUND_TYPE::BACKGROUND3: return L"background\\background3.bmp";
    default: return L"";
    }
}

void CBackgroundFactory::SetupBackgroundProperties(CBackground* _pBackground, BACKGROUND_TYPE _eType)
{
    if (!_pBackground) return;

    // 배경 타입별 특성 설정
    switch (_eType)
    {
    case BACKGROUND_TYPE::STATIC:
        _pBackground->SetScrollable(false);
        _pBackground->SetScrollSpeed(Vec2(0.f, 0.f));
        break;
    case BACKGROUND_TYPE::SCROLLABLE:
        _pBackground->SetScrollable(true);
        _pBackground->SetScrollSpeed(Vec2(50.f, 0.f));
        break;
    case BACKGROUND_TYPE::PARALLAX:
        _pBackground->SetScrollable(true);
        _pBackground->SetScrollSpeed(Vec2(25.f, 0.f));
        break;
    case BACKGROUND_TYPE::BACKGROUND1:
    case BACKGROUND_TYPE::BACKGROUND2:
    case BACKGROUND_TYPE::BACKGROUND3:
        // 실제 배경 이미지들은 기본적으로 정적 배경으로 설정
        _pBackground->SetScrollable(false);
        _pBackground->SetScrollSpeed(Vec2(0.f, 0.f));
        break;
    }
}