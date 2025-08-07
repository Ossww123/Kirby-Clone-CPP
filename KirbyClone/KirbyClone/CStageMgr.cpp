#include "pch.h"
#include "CStageMgr.h"
#include "CStageImage.h"
#include "CResMgr.h"
#include "CTexture.h"

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

CStageImage* CStageMgr::CreateStageImage(STAGE_IMAGE_TYPE _eType, const wstring& _strTexturePath)
{
    // 이미 존재하는 스테이지 이미지인지 확인
    CStageImage* pExistStage = FindStageImage(_eType);
    if (pExistStage)
        return pExistStage;

    // 텍스처 로드
    wstring strKey = L"StageImage_";

    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:      strKey += L"Stage01"; break;
    case STAGE_IMAGE_TYPE::STAGE_02:      strKey += L"Stage02"; break;
    case STAGE_IMAGE_TYPE::CUSTOM:        strKey += L"Custom"; break;
    default:                              strKey += L"Unknown"; break;
    }

    CTexture* pTexture = CResMgr::GetInst()->LoadTexture(strKey, _strTexturePath);

    if (!pTexture)
    {
        pTexture = new CTexture;
        if (SUCCEEDED(pTexture->LoadWithAlpha(_strTexturePath)))
        {
            pTexture->SetKey(strKey);
            pTexture->SetRelativePath(_strTexturePath);
            CResMgr::GetInst()->AddTexture(strKey, pTexture);
        }
        else
        {
            delete pTexture;
            pTexture = nullptr;
        }
    }

    if (!pTexture)
    {
        MessageBox(nullptr, L"스테이지 이미지 텍스처 로드 실패", L"에러", MB_OK);
        return nullptr;
    }

    // 새 스테이지 이미지 생성
    CStageImage* pNewStage = new CStageImage;
    pNewStage->SetStageTexture(pTexture);
    pNewStage->SetupStageImage(_eType);

    // 맵에 추가
    m_mapStageImage.insert(make_pair(_eType, pNewStage));

    return pNewStage;
}

CStageImage* CStageMgr::FindStageImage(STAGE_IMAGE_TYPE _eType)
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

STAGE_IMAGE_TYPE CStageMgr::GetCurrentStageType()
{
    if (m_pCurrentStageImage)
        return m_pCurrentStageImage->GetStageType();

    return STAGE_IMAGE_TYPE::STAGE_01; // 기본값
}

const wchar_t* CStageMgr::GetStageImageName(STAGE_IMAGE_TYPE _eType)
{
    switch (_eType)
    {
    case STAGE_IMAGE_TYPE::STAGE_01:      return L"Green Hill Stage";
    case STAGE_IMAGE_TYPE::STAGE_02:      return L"Castle Stage";
    case STAGE_IMAGE_TYPE::CUSTOM:        return L"Custom Stage";
    default:                              return L"Unknown Stage";
    }
}

vector<STAGE_IMAGE_TYPE> CStageMgr::GetAvailableStageImageTypes()
{
    vector<STAGE_IMAGE_TYPE> result;

    result.push_back(STAGE_IMAGE_TYPE::STAGE_01);
    result.push_back(STAGE_IMAGE_TYPE::STAGE_02);

    // 커스텀 스테이지가 로드되어 있으면 추가
    if (FindStageImage(STAGE_IMAGE_TYPE::CUSTOM))
    {
        result.push_back(STAGE_IMAGE_TYPE::CUSTOM);
    }

    return result;
}

bool CStageMgr::LoadCustomStageImage(const wstring& _strFilePath)
{
    // 커스텀 스테이지 이미지 로드
    CStageImage* pCustomStage = CreateStageImage(STAGE_IMAGE_TYPE::CUSTOM, _strFilePath);

    if (pCustomStage)
    {
        // 성공적으로 로드되면 현재 스테이지로 설정
        SetCurrentStageImage(STAGE_IMAGE_TYPE::CUSTOM);
        return true;
    }

    return false;
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

void CStageMgr::CreateDefaultStageImages()
{
    // 기본 스테이지 이미지들 생성
    CreateStageImage(STAGE_IMAGE_TYPE::STAGE_01, L"stage\\stage01_full.bmp");
    CreateStageImage(STAGE_IMAGE_TYPE::STAGE_02, L"stage\\stage02_full.bmp");

    // 기본 스테이지 설정
    SetCurrentStageImage(STAGE_IMAGE_TYPE::STAGE_01);
}

STAGE_IMAGE_TYPE CStageMgr::GenerateCustomStageType()
{
    // 현재는 하나의 커스텀 타입만 지원
    // 필요시 여러 커스텀 스테이지를 위한 동적 타입 생성 로직 추가 가능
    return STAGE_IMAGE_TYPE::CUSTOM;
}