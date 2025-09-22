#include "gamePCH.h"
#include "CTexture.h"
#include "CCore.h"

#pragma comment(lib, "msimg32.lib")  // AlphaBlend 함수를 위해 필요

CTexture::CTexture()
    : m_dc(0)
    , m_hBit(0)
    , m_tInfo{}
    , m_bHasAlpha(false)
    , m_pPixelData(nullptr)
{
}

CTexture::~CTexture()
{
    CleanupPixelData();
    DeleteDC(m_dc);
    DeleteObject(m_hBit);
}

HRESULT CTexture::Load(const wstring& _strFilePath)
{
    // 기존 호환성을 위한 함수 - 24비트 BMP 로드
    return Load24BitBMP(_strFilePath);
}

HRESULT CTexture::LoadWithAlpha(const wstring& _strFilePath)
{
    // 파일 확장자에 따라 적절한 로드 방식 선택
    wstring ext = _strFilePath.substr(_strFilePath.find_last_of(L"."));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    if (ext == L".bmp")
    {
        // BMP 파일의 경우 단계적으로 로딩 시도

        // 1단계: 32비트 방식으로 시도
        HRESULT hr32 = Load32BitBMP(_strFilePath);
        if (SUCCEEDED(hr32))
        {
            return S_OK;
        }

        // 2단계: 32비트 실패시 24비트 방식으로 시도
        HRESULT hr24 = Load24BitBMP(_strFilePath);
        if (SUCCEEDED(hr24))
        {
            return S_OK;
        }

        // 3단계: 둘 다 실패시 에러
        wchar_t szError[512];
        swprintf_s(szError, L"BMP 파일 로딩 완전 실패\n경로: %s\n\n32비트 시도: 실패\n24비트 시도: 실패\n\n파일이 유효한 BMP인지 확인해주세요.", _strFilePath.c_str());
        MessageBox(nullptr, szError, L"BMP 로딩 실패", MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    // 다른 형식은 24비트로 시도
    return Load24BitBMP(_strFilePath);
}

HRESULT CTexture::Load24BitBMP(const wstring& _strFilePath)
{
    // 기존 로드 방식 (24비트 BMP)
    m_bHasAlpha = false;

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

HRESULT CTexture::Load32BitBMP(const wstring& _strFilePath)
{
    // 파일 존재 여부 먼저 확인
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(_strFilePath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        wchar_t szBuffer[512] = {};
        swprintf_s(szBuffer, L"32비트 BMP 로딩 실패 - 파일을 찾을 수 없음\n경로: %s", _strFilePath.c_str());
        MessageBox(nullptr, szBuffer, L"파일 없음", MB_OK);
        return E_FAIL;
    }
    FindClose(hFind);

    // LoadImage로 비트맵 로드 시도
    m_hBit = (HBITMAP)LoadImage(nullptr, _strFilePath.c_str(), IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    if (nullptr == m_hBit)
    {
        // LoadImage 실패 원인 확인
        DWORD dwError = GetLastError();
        wchar_t szBuffer[512] = {};
        swprintf_s(szBuffer, L"32비트 BMP LoadImage 실패\n경로: %s\n오류 코드: %d\n\n가능한 원인:\n- 파일이 실제로 32비트가 아님\n- 파일이 손상됨\n- 비표준 BMP 형식",
            _strFilePath.c_str(), dwError);
        MessageBox(nullptr, szBuffer, L"LoadImage 실패", MB_OK);
        return E_FAIL;
    }

    // 비트맵 정보 얻기
    GetObject(m_hBit, sizeof(BITMAP), &m_tInfo);

    // 실제 비트 수 확인 및 디버깅 정보 출력
    wchar_t szDebugInfo[512];
    swprintf_s(szDebugInfo, L"BMP 파일 로드 성공!\n\n파일: %s\n크기: %d x %d\n비트 수: %d\n평면 수: %d\n바이트/라인: %d",
        _strFilePath.c_str(),
        m_tInfo.bmWidth, m_tInfo.bmHeight,
        m_tInfo.bmBitsPixel, m_tInfo.bmPlanes, m_tInfo.bmWidthBytes);

    // 디버깅 정보 표시 (임시)
    MessageBox(nullptr, szDebugInfo, L"BMP 로딩 디버그", MB_OK);

    // 32비트인지 확인
    if (m_tInfo.bmBitsPixel == 32)
    {
        m_bHasAlpha = true;

        // DC 생성
        m_dc = CreateCompatibleDC(CCore::GetInst()->GetMainDC());
        HBITMAP hPrevBit = (HBITMAP)SelectObject(m_dc, m_hBit);
        DeleteObject(hPrevBit);

        // 알파 채널 전처리
        PreprocessAlpha();
    }
    else
    {
        // 32비트가 아니면 24비트 방식으로 폴백
        wchar_t szBuffer[256];
        swprintf_s(szBuffer, L"파일이 32비트가 아닙니다 (실제: %d비트)\n24비트 방식으로 처리합니다.", m_tInfo.bmBitsPixel);
        MessageBox(nullptr, szBuffer, L"비트 수 불일치", MB_OK);

        m_bHasAlpha = false;
        m_dc = CreateCompatibleDC(CCore::GetInst()->GetMainDC());
        HBITMAP hPrevBit = (HBITMAP)SelectObject(m_dc, m_hBit);
        DeleteObject(hPrevBit);
    }

    return S_OK;
}

void CTexture::PreprocessAlpha()
{
    if (!m_bHasAlpha) return;

    // 32비트 비트맵의 알파 채널 전처리
    // Windows의 AlphaBlend는 Premultiplied Alpha를 사용하므로 필요시 전처리

    // 픽셀 데이터 접근을 위한 DIBSECTION 생성
    DIBSECTION dibSection;
    if (GetObject(m_hBit, sizeof(DIBSECTION), &dibSection) == sizeof(DIBSECTION))
    {
        m_pPixelData = (BYTE*)dibSection.dsBm.bmBits;

        // 필요한 경우 알파 전처리 로직 추가
        // 예: RGB 값에 알파를 미리 곱하기 (Premultiplied Alpha)
        /*
        int pixelCount = m_tInfo.bmWidth * m_tInfo.bmHeight;
        DWORD* pixels = (DWORD*)m_pPixelData;

        for (int i = 0; i < pixelCount; ++i)
        {
            BYTE alpha = (pixels[i] >> 24) & 0xFF;
            if (alpha < 255)
            {
                BYTE r = ((pixels[i] >> 16) & 0xFF) * alpha / 255;
                BYTE g = ((pixels[i] >> 8) & 0xFF) * alpha / 255;
                BYTE b = (pixels[i] & 0xFF) * alpha / 255;

                pixels[i] = (alpha << 24) | (r << 16) | (g << 8) | b;
            }
        }
        */
    }
}

void CTexture::RenderWithAlpha(HDC _dc, Vec2 _vPos, Vec2 _vScale, float _fAlpha)
{
    if (!m_bHasAlpha)
    {
        // 알파 채널이 없으면 기존 방식으로 폴백
        RenderWithColorKey(_dc, _vPos, _vScale);
        return;
    }

    // 알파 블렌딩 설정
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(_fAlpha * 255);
    blend.AlphaFormat = AC_SRC_ALPHA;  // 소스에 알파 채널 있음

    // 렌더링 위치 계산
    int destX = (int)(_vPos.x - _vScale.x / 2.f);
    int destY = (int)(_vPos.y - _vScale.y / 2.f);

    // 알파 블렌딩으로 렌더링
    AlphaBlend(_dc,
        destX, destY,
        (int)_vScale.x, (int)_vScale.y,
        m_dc,
        0, 0,
        m_tInfo.bmWidth, m_tInfo.bmHeight,
        blend);
}

void CTexture::RenderWithAlpha(HDC _dc, int x, int y, int width, int height, float _fAlpha)
{
    if (!m_bHasAlpha)
    {
        RenderWithColorKey(_dc, x, y, width, height);
        return;
    }

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(_fAlpha * 255);
    blend.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(_dc,
        x, y, width, height,
        m_dc,
        0, 0,
        m_tInfo.bmWidth, m_tInfo.bmHeight,
        blend);
}

void CTexture::RenderWithColorKey(HDC _dc, Vec2 _vPos, Vec2 _vScale, COLORREF _keyColor)
{
    // 기존 마젠타 키 색상 방식
    TransparentBlt(_dc,
        (int)(_vPos.x - _vScale.x / 2.f),
        (int)(_vPos.y - _vScale.y / 2.f),
        (int)_vScale.x,
        (int)_vScale.y,
        m_dc,
        0, 0,
        m_tInfo.bmWidth,
        m_tInfo.bmHeight,
        _keyColor);
}

void CTexture::RenderWithColorKey(HDC _dc, int x, int y, int width, int height, COLORREF _keyColor)
{
    TransparentBlt(_dc,
        x, y, width, height,
        m_dc,
        0, 0,
        m_tInfo.bmWidth,
        m_tInfo.bmHeight,
        _keyColor);
}

void CTexture::RenderSpriteWithAlpha(HDC _dc, Vec2 _vPos, Vec2 _vSrcLT, Vec2 _vSrcSize, Vec2 _vDestSize, float _fAlpha)
{
    if (!m_bHasAlpha)
    {
        RenderSpriteWithColorKey(_dc, _vPos, _vSrcLT, _vSrcSize, _vDestSize);
        return;
    }

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(_fAlpha * 255);
    blend.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(_dc,
        (int)(_vPos.x - _vDestSize.x / 2.f),
        (int)(_vPos.y - _vDestSize.y / 2.f),
        (int)_vDestSize.x,
        (int)_vDestSize.y,
        m_dc,
        (int)_vSrcLT.x,
        (int)_vSrcLT.y,
        (int)_vSrcSize.x,
        (int)_vSrcSize.y,
        blend);
}

void CTexture::RenderSpriteWithColorKey(HDC _dc, Vec2 _vPos, Vec2 _vSrcLT, Vec2 _vSrcSize, Vec2 _vDestSize, COLORREF _keyColor)
{
    TransparentBlt(_dc,
        (int)(_vPos.x - _vDestSize.x / 2.f),
        (int)(_vPos.y - _vDestSize.y / 2.f),
        (int)_vDestSize.x,
        (int)_vDestSize.y,
        m_dc,
        (int)_vSrcLT.x,
        (int)_vSrcLT.y,
        (int)_vSrcSize.x,
        (int)_vSrcSize.y,
        _keyColor);
}

void CTexture::CleanupPixelData()
{
    // DIBSECTION에서 가져온 픽셀 데이터는 따로 해제할 필요 없음
    // (비트맵과 함께 자동 해제됨)
    m_pPixelData = nullptr;
}