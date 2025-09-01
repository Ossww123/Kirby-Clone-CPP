#include "pch.h"
#include "CStageMgr.h"
#include "CStageImage.h"
#include "CResMgr.h"
#include "CTexture.h"
#include "CPathMgr.h"
#include "CCore.h"

CStageMgr::CStageMgr()
    : m_pCurrentStageImage(nullptr)
{
}

CStageMgr::~CStageMgr()
{
    // 모든 스테이지 이미지 해제
    for (auto& pair : m_mapStageImage)
    {
        delete pair.second;
    }
    m_mapStageImage.clear();
}

void CStageMgr::init()
{
    CreateDefaultStageImages();
}


void CStageMgr::Update()
{
    if (m_pCurrentStageImage)
    {
        m_pCurrentStageImage->Update();
    }
}

void CStageMgr::Render(HDC _dc)
{
    if (m_pCurrentStageImage)
    {
        m_pCurrentStageImage->Render(_dc);
    }
}

CStageImage* CStageMgr::CreateStageImage(STAGE_IMAGE_TYPE _eType, const wstring& _strTexturePath)
{
    // 이미 존재하는 스테이지 이미지인지 확인
    CStageImage* pExistStage = FindStageImage(_eType);
    if (pExistStage)
        return pExistStage;

    // 텍스처 키 생성
    wstring strKey = L"StageImage_";

    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:      strKey += L"Stage01"; break;
    case STAGE_IMAGE_TYPE::STAGE_02:      strKey += L"Stage02"; break;
    case STAGE_IMAGE_TYPE::STAGE_03:      strKey += L"Stage03"; break;
    case STAGE_IMAGE_TYPE::CUSTOM:        strKey += L"Custom"; break;
    default:                              strKey += L"Unknown"; break;
    }

    // 24비트 BMP 로딩 (마젠타 컬러키 방식)
    CTexture* pTexture = CResMgr::GetInst()->LoadTexture(strKey, _strTexturePath);

    if (!pTexture)
    {
        return nullptr;
    }

    // 새 스테이지 이미지 생성
    CStageImage* pNewStage = new CStageImage;
    pNewStage->SetStageTexture(pTexture);

    // 텍스처를 먼저 설정한 후 SetupStageImage 호출
    // 이렇게 하면 SetImageToBottomLeft에서 텍스처 정보를 사용할 수 있음
    pNewStage->SetupStageImage(_eType);

    // 맵에 추가
    m_mapStageImage.insert(make_pair(_eType, pNewStage));

    return pNewStage;
}

CStageImage* CStageMgr::FindStageImage(STAGE_IMAGE_TYPE _eType) const
{
    auto iter = m_mapStageImage.find(_eType);

    if (iter == m_mapStageImage.end())
        return nullptr;

    return iter->second;
}

void CStageMgr::SetCurrentStageImage(STAGE_IMAGE_TYPE _eType)
{
    m_pCurrentStageImage = FindStageImage(_eType);
}

void CStageMgr::CreateDefaultStageImages()
{
    // 기본 스테이지 이미지들 생성
    CreateStageImage(STAGE_IMAGE_TYPE::STAGE_01, L"stage\\stage01_full.bmp");
    CreateStageImage(STAGE_IMAGE_TYPE::STAGE_02, L"stage\\stage02_full.bmp");
    CreateStageImage ( STAGE_IMAGE_TYPE::STAGE_03 , L"stage\\stage03_full.bmp" );

    // 기본 스테이지 설정
    SetCurrentStageImage(STAGE_IMAGE_TYPE::STAGE_01);
}

STAGE_IMAGE_TYPE CStageMgr::GetCurrentStageType() const
{
    if (m_pCurrentStageImage)
        return m_pCurrentStageImage->GetStageType();

    return STAGE_IMAGE_TYPE::STAGE_01; // 기본값
}

const wchar_t* CStageMgr::GetStageImageName(STAGE_IMAGE_TYPE _eType) const
{
    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:      return L"Stage01";
    case STAGE_IMAGE_TYPE::STAGE_02:      return L"Stage02";
    case STAGE_IMAGE_TYPE::STAGE_03:      return L"Stage03";
    case STAGE_IMAGE_TYPE::CUSTOM:        return L"Custom Stage";
    default:                              return L"Unknown Stage";
    }
}

vector<STAGE_IMAGE_TYPE> CStageMgr::GetAvailableStageImageTypes() const
{
    vector<STAGE_IMAGE_TYPE> result;

    result.push_back(STAGE_IMAGE_TYPE::STAGE_01);
    result.push_back(STAGE_IMAGE_TYPE::STAGE_02);
    result.push_back ( STAGE_IMAGE_TYPE::STAGE_03 );

    // 커스텀 스테이지가 로드되어 있으면 추가
    if (FindStageImage(STAGE_IMAGE_TYPE::CUSTOM))
    {
        result.push_back(STAGE_IMAGE_TYPE::CUSTOM);
    }

    return result;
}