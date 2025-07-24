#pragma once
#include "CRes.h"

class CTexture : public CRes
{
private:
    HDC     m_dc;           // 텍스처 DC
    HBITMAP m_hBit;         // 비트맵 핸들
    BITMAP  m_tInfo;        // 비트맵 정보

public:
    HRESULT Load(const wstring& _strFilePath);

    UINT GetWidth() { return m_tInfo.bmWidth; }
    UINT GetHeight() { return m_tInfo.bmHeight; }
    HDC GetDC() { return m_dc; }

public:
    CTexture();
    ~CTexture();
};