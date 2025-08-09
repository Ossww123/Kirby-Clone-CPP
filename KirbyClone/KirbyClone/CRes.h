#pragma once

// 리소스 관리 기본 클래스
class CRes
{
public:
    // === 생명주기 함수 ===
    CRes();
    virtual ~CRes();

public:
    // === 리소스 정보 설정 ===
    void SetKey(const wstring& _strKey) { m_strKey = _strKey; }
    void SetRelativePath(const wstring& _strPath) { m_strRelativePath = _strPath; }

public:
    // === 리소스 정보 접근자 ===
    const wstring& GetKey() const { return m_strKey; }
    const wstring& GetRelativePath() const { return m_strRelativePath; }

private:
    // === 멤버 변수들 ===
    wstring m_strKey;           // 리소스 키값
    wstring m_strRelativePath;  // 상대 경로
};