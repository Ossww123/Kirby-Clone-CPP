#pragma once
#include "CRes.h"

class CTexture : public CRes
{
private:
    HDC     m_dc;           // 텍스처 DC
    HBITMAP m_hBit;         // 비트맵 핸들
    BITMAP  m_tInfo;        // 비트맵 정보

    // 알파 채널 지원 관련
    bool    m_bHasAlpha;    // 알파 채널 보유 여부
    BYTE* m_pPixelData;   // 32비트 픽셀 데이터 (알파 채널 포함)

public:
    // 기존 로드 함수 (24비트 BMP, 마젠타 호환용)
    HRESULT Load(const wstring& _strFilePath);

    // 새로운 알파 채널 지원 로드 함수
    HRESULT LoadWithAlpha(const wstring& _strFilePath);

    // 렌더링 함수들
    void RenderWithAlpha(HDC _dc, Vec2 _vPos, Vec2 _vScale, float _fAlpha = 1.0f);
    void RenderWithAlpha(HDC _dc, int x, int y, int width, int height, float _fAlpha = 1.0f);
    void RenderWithColorKey(HDC _dc, Vec2 _vPos, Vec2 _vScale, COLORREF _keyColor = RGB(255, 0, 255));
    void RenderWithColorKey(HDC _dc, int x, int y, int width, int height, COLORREF _keyColor = RGB(255, 0, 255));

    // 스프라이트 렌더링 (애니메이션용)
    void RenderSpriteWithAlpha(HDC _dc, Vec2 _vPos, Vec2 _vSrcLT, Vec2 _vSrcSize, Vec2 _vDestSize, float _fAlpha = 1.0f);
    void RenderSpriteWithColorKey(HDC _dc, Vec2 _vPos, Vec2 _vSrcLT, Vec2 _vSrcSize, Vec2 _vDestSize, COLORREF _keyColor = RGB(255, 0, 255));

    // 정보 함수들
    UINT GetWidth() { return m_tInfo.bmWidth; }
    UINT GetHeight() { return m_tInfo.bmHeight; }
    HDC GetDC() { return m_dc; }
    bool HasAlpha() { return m_bHasAlpha; }
    int GetBitsPerPixel() { return m_tInfo.bmBitsPixel; }

private:
    // 내부 헬퍼 함수들
    HRESULT Load24BitBMP(const wstring& _strFilePath);    // 24비트 BMP 로드
    HRESULT Load32BitBMP(const wstring& _strFilePath);    // 32비트 BMP 로드
    void PreprocessAlpha();                               // 알파 채널 전처리
    void CleanupPixelData();                             // 픽셀 데이터 정리

public:
    CTexture();
    ~CTexture();
};