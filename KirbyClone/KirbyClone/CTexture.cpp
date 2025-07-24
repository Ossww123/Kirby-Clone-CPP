#include "pch.h"
#include "CTexture.h"
#include "CCore.h"

CTexture::CTexture()
    : m_dc(0)
    , m_hBit(0)
    , m_tInfo{}
{
}

CTexture::~CTexture()
{
    DeleteDC(m_dc);
    DeleteObject(m_hBit);
}

HRESULT CTexture::Load(const wstring& _strFilePath)
{
    // 비트맵 파일 로드
    m_hBit = (HBITMAP)LoadImage(nullptr, _strFilePath.c_str(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    if (nullptr == m_hBit)
    {
        wchar_t szBuffer[256] = {};
        swprintf_s(szBuffer, L"비트맵 로드 실패\n경로: %s", _strFilePath.c_str());
        MessageBox(nullptr, szBuffer, L"텍스처 로드 실패", MB_OK);
        return E_FAIL;
    }

    // 로드된 비트맵과 호환되는 DC 생성
    m_dc = CreateCompatibleDC(CCore::GetInst()->GetMainDC());

    // 비트맵과 DC 연결
    HBITMAP hPrevBit = (HBITMAP)SelectObject(m_dc, m_hBit);
    DeleteObject(hPrevBit);

    // 비트맵 정보 얻기
    GetObject(m_hBit, sizeof(BITMAP), &m_tInfo);

    return S_OK;
}