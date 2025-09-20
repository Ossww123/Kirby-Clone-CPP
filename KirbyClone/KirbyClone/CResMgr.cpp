#include "gamePCH.h"
#include "CResMgr.h"

#include "CPathMgr.h"
#include "CTexture.h"

CResMgr::CResMgr()
{
}

CResMgr::~CResMgr()
{
    // 모든 리소스 해제
    map<wstring, CRes*>::iterator iter = m_mapTex.begin();
    for (; iter != m_mapTex.end(); ++iter)
    {
        delete iter->second;
    }
}

void CResMgr::init()
{
    CreateDefaultTexture();
}

CTexture* CResMgr::LoadTexture(const wstring& _strKey, const wstring& _strRelativePath)
{
    // 이미 로드된 텍스처가 있는지 확인
    CTexture* pTex = FindTexture(_strKey);
    if (nullptr != pTex)
    {
        return pTex;
    }

    // 새로운 텍스처 생성 및 로드
    wstring strFilePath = CPathMgr::GetInst()->GetContentPath();
    strFilePath += _strRelativePath;

    pTex = new CTexture;
    if (FAILED(pTex->Load(strFilePath)))
    {
        delete pTex;
        MessageBox(nullptr, L"텍스처 로드 실패", L"리소스 로드 실패", MB_OK);
        return nullptr;
    }

    pTex->SetKey(_strKey);
    pTex->SetRelativePath(_strRelativePath);
    m_mapTex.insert(make_pair(_strKey, pTex));

    return pTex;
}

CTexture* CResMgr::FindTexture(const wstring& _strKey)
{
    map<wstring, CRes*>::iterator iter = m_mapTex.find(_strKey);

    if (iter == m_mapTex.end())
    {
        return nullptr;
    }

    return (CTexture*)iter->second;
}

void CResMgr::AddTexture(const wstring& _strKey, CTexture* _pTexture)
{
    // 이미 존재하는 키인지 확인
    if (FindTexture(_strKey))
    {
        MessageBox(nullptr, L"이미 존재하는 텍스처 키입니다.", L"리소스 매니저 오류", MB_OK);
        return;
    }

    m_mapTex.insert(make_pair(_strKey, _pTexture));
}

CTexture* CResMgr::LoadTextureWithAlpha(const wstring& _strKey, const wstring& _strRelativePath)
{
    // 이미 로드된 텍스처가 있는지 확인
    CTexture* pTex = FindTexture(_strKey);
    if (nullptr != pTex)
    {
        return pTex;
    }

    // 새로운 텍스처 생성 및 알파 채널 지원 로드
    wstring strFilePath = CPathMgr::GetInst()->GetContentPath();
    strFilePath += _strRelativePath;

    pTex = new CTexture;
    if (FAILED(pTex->LoadWithAlpha(strFilePath)))
    {
        delete pTex;
        MessageBox(nullptr, L"알파 채널 텍스처 로드 실패", L"리소스 로드 실패", MB_OK);
        return nullptr;
    }

    pTex->SetKey(_strKey);
    pTex->SetRelativePath(_strRelativePath);
    m_mapTex.insert(make_pair(_strKey, pTex));

    return pTex;
}

void CResMgr::CreateDefaultTexture()
{
    // 기본 텍스처들을 여기서 미리 로드할 수 있습니다.
    // 예: LoadTexture(L"DefaultPlayer", L"texture\\player.bmp");
}