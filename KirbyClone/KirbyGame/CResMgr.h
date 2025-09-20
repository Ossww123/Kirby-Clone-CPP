#pragma once

class CRes;
class CTexture;

class CResMgr
{
    SINGLE(CResMgr);

private:
    map<wstring, CRes*> m_mapTex;       // 텍스처 리소스 맵

public:
    void init();

    CTexture* LoadTexture(const wstring& _strKey, const wstring& _strRelativePath);
    CTexture* FindTexture(const wstring& _strKey);

    void AddTexture(const wstring& _strKey, CTexture* _pTexture);
    CTexture* LoadTextureWithAlpha(const wstring& _strKey, const wstring& _strRelativePath);

private:
    void CreateDefaultTexture();       // 기본 텍스처 생성
};