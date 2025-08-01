#include "pch.h"
#include "CBackgroundMgr.h"
#include "CBackground.h"
#include "CResMgr.h"
#include "CTexture.h"

CBackgroundMgr::CBackgroundMgr()
{
}

CBackgroundMgr::~CBackgroundMgr()
{
    // 모든 배경 해제
    for (auto& pair : m_mapBackground)
    {
        delete pair.second;
    }
    m_mapBackground.clear();
}

void CBackgroundMgr::init()
{
    CreateDefaultBackgrounds();
}

CBackground* CBackgroundMgr::CreateBackground(BACKGROUND_TYPE _eType, const wstring& _strTexturePath)
{
    // 이미 존재하는 배경인지 확인
    CBackground* pExistBg = FindBackground(_eType);
    if (pExistBg)
        return pExistBg;

    // 텍스처 로드
    wstring strKey = L"Background_";

    switch (_eType)
    {
    case BACKGROUND_TYPE::GREEN_HILL:      strKey += L"GreenHill"; break;
    case BACKGROUND_TYPE::RAINBOW_CASTLE:  strKey += L"RainbowCastle"; break;
    default:                               strKey += L"Unknown"; break;
    }

    CTexture* pTexture = CResMgr::GetInst()->LoadTexture(strKey, _strTexturePath);

    if (!pTexture)
    {
        MessageBox(nullptr, L"배경 텍스처 로드 실패", L"에러", MB_OK);
        return nullptr;
    }

    // 새 배경 생성
    CBackground* pNewBg = new CBackground;
    pNewBg->SetBackgroundTexture(pTexture);
    pNewBg->SetupBackground(_eType);

    // 맵에 추가
    m_mapBackground.insert(make_pair(_eType, pNewBg));

    return pNewBg;
}

CBackground* CBackgroundMgr::FindBackground(BACKGROUND_TYPE _eType)
{
    auto iter = m_mapBackground.find(_eType);

    if (iter == m_mapBackground.end())
        return nullptr;

    return iter->second;
}

const wchar_t* CBackgroundMgr::GetBackgroundName(BACKGROUND_TYPE _eType)
{
    switch (_eType)
    {
    case BACKGROUND_TYPE::GREEN_HILL:      return L"Green Hill";
    case BACKGROUND_TYPE::RAINBOW_CASTLE:  return L"Rainbow Castle";
    default:                               return L"Unknown";
    }
}

vector<BACKGROUND_TYPE> CBackgroundMgr::GetAvailableBackgroundTypes()
{
    vector<BACKGROUND_TYPE> result;

    result.push_back(BACKGROUND_TYPE::GREEN_HILL);
    result.push_back(BACKGROUND_TYPE::RAINBOW_CASTLE);

    return result;
}

void CBackgroundMgr::CreateDefaultBackgrounds()
{
    // 기본 배경들 생성
    CreateBackground(BACKGROUND_TYPE::GREEN_HILL, L"background\\green_hill.bmp");
    CreateBackground(BACKGROUND_TYPE::RAINBOW_CASTLE, L"background\\rainbow_castle.bmp");
}