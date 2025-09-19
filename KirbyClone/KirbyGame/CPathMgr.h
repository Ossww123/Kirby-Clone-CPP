#pragma once

class CPathMgr
{
    SINGLE(CPathMgr);

private:
    wstring m_strContentPath;   // Content 폴더 경로
    wstring m_strRelativePath;  // 상대 경로

public:
    void init();

    const wstring& GetContentPath() { return m_strContentPath; }
    wstring GetRelativePath(const wstring& _strFilePath);
};